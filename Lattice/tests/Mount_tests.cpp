#include <stdexcept>

#include <Lattice/tests/RuntimeFixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice::MountTests {

struct SharedOwner { virtual ~SharedOwner() = default; };
struct Owner final : SharedOwner { explicit Owner(NodeBuild) {} };

struct ResourceDesc { int value = 0; };

struct Resource {
    using Desc = ResourceDesc;
    static inline int constructed = 0;
    static inline int configured = 0;
    static inline int destroyed = 0;
    int value = 0;

    Resource(NodeBuild, const Desc& desc) : value(desc.value) { ++constructed; }
    void configure(NodeConfigure) { ++configured; }
    ~Resource() { ++destroyed; }
};

struct PlainResource {
    static inline int constructed = 0;
    explicit PlainResource(NodeBuild) { ++constructed; }
};

struct ThrowingResource {
    using Desc = ResourceDesc;
    ThrowingResource(NodeBuild node, const Desc&) {
        node.add<PlainResource>("temporary");
        throw std::runtime_error("resource construction failed");
    }
};

void registerTypes(RuntimeFixture& fixture) {
    auto& blueprints = fixture.run_ctx.blueprints;
    BlueprintRegister::add<SharedOwner>(blueprints);
    BlueprintRegister::add<Owner, SharedOwner>(blueprints);
    BlueprintRegister::add<Resource>(blueprints);
    BlueprintRegister::add<PlainResource>(blueprints);
    BlueprintRegister::add<ThrowingResource>(blueprints);
}

void resetCounters() {
    Resource::constructed = 0;
    Resource::configured = 0;
    Resource::destroyed = 0;
    PlainResource::constructed = 0;
}

NodeId createOwner(RuntimeFixture& fixture) {
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = nodes.factory.component(
        fixture.root, fixture.run_ctx.blueprints.id<Owner>(), "owner"
    );
    nodes.factory.share(owner, fixture.run_ctx.blueprints.id<SharedOwner>());
    return owner;
}

TEST(Node_CompactLayout, RuntimeFixture,
    "Node должен занимать 64 байта на 64-битной платформе.")
{
    if constexpr (sizeof(void*) == 8)
        REQUIRE(sizeof(Node) == 64);
}

TEST(Mount_AddLocalIdentityAndLifetime, RuntimeFixture,
    "Local resource уникален для caller и живёт не дольше caller и physical owner.")
{
    registerTypes(fixture);
    resetCounters();
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = createOwner(fixture);
    const NodeId callerA = nodes.factory.folder(fixture.root, "caller-a");
    const NodeId callerB = nodes.factory.folder(fixture.root, "caller-b");

    auto mountA = fixture.build(callerA).mount<SharedOwner>();
    auto mountB = fixture.build(callerB).mount<SharedOwner>();
    auto first = mountA.addLocal<Resource>("buffer", ResourceDesc{11});
    auto repeated = mountA.addLocal<Resource>("buffer", ResourceDesc{99});
    auto other = mountB.addLocal<Resource>("buffer", ResourceDesc{22});

    REQUIRE(first.get() == repeated.get());
    REQUIRE(first.get() != other.get());
    REQUIRE(first->value == 11);
    REQUIRE(other->value == 22);
    REQUIRE(Resource::constructed == 2);

    const auto api = fixture.run_ctx.blueprints.id<Resource>();
    const NodeId referenceA = nodes.registry.find("buffer", callerA, api, owner);
    const NodeId referenceB = nodes.registry.find("buffer", callerB, api, owner);
    REQUIRE(nodes.registry.require(referenceA).kind == NodeKind::Mount);
    REQUIRE(nodes.registry.require(referenceB).kind == NodeKind::Mount);
    REQUIRE(nodes.registry.require(nodes.registry.require(referenceA).relation).parent == owner);
    REQUIRE(nodes.configure(callerA).require<Resource>("buffer").get() == first.get());

    nodes.ops.configureBranch(callerA);
    nodes.ops.configureBranch(callerA);
    REQUIRE(Resource::configured == 1);

    const NodeId physicalA = nodes.registry.require(referenceA).relation;
    const NodeId physicalB = nodes.registry.require(referenceB).relation;
    REQUIRE(nodes.registry.references(physicalA).size() == 1);
    REQUIRE(nodes.registry.references(physicalB).size() == 1);
    nodes.ops.destroyBranch(callerA);
    REQUIRE(nodes.registry.get(physicalA) == nullptr);
    REQUIRE(nodes.registry.get(physicalB) != nullptr);
    REQUIRE(Resource::destroyed == 1);

    nodes.ops.destroyBranch(owner);
    REQUIRE(nodes.registry.get(referenceB) == nullptr);
    REQUIRE(nodes.registry.get(physicalB) == nullptr);
    REQUIRE(Resource::destroyed == 2);
}

TEST(Mount_AddShareCountsUniqueCallers, RuntimeFixture,
    "Shared resource учитывает одну ссылку на caller и удаляется после последнего caller.")
{
    registerTypes(fixture);
    resetCounters();
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = createOwner(fixture);
    const NodeId callerA = nodes.factory.folder(fixture.root, "caller-a");
    const NodeId callerB = nodes.factory.folder(fixture.root, "caller-b");

    auto mountA = fixture.build(callerA).mount<SharedOwner>();
    auto mountB = fixture.build(callerB).mount<SharedOwner>();
    auto first = mountA.addShare<Resource>("mesh", ResourceDesc{7});
    auto repeated = mountA.addShare<Resource>("mesh", ResourceDesc{99});
    auto secondCaller = mountB.addShare<Resource>("mesh", ResourceDesc{42});

    REQUIRE(first.get() == repeated.get());
    REQUIRE(first.get() == secondCaller.get());
    REQUIRE(first->value == 7);
    REQUIRE(Resource::constructed == 1);

    const auto api = fixture.run_ctx.blueprints.id<Resource>();
    const NodeId referenceA = nodes.registry.find("mesh", callerA, api, owner);
    const NodeId referenceB = nodes.registry.find("mesh", callerB, api, owner);
    const NodeId physical = nodes.registry.require(referenceA).relation;
    REQUIRE(nodes.registry.require(referenceA).kind == NodeKind::SharedMount);
    REQUIRE(nodes.registry.require(referenceB).relation == physical);
    REQUIRE(nodes.registry.references(physical).size() == 2);

    nodes.ops.destroyBranch(callerA);
    REQUIRE(nodes.registry.get(physical) != nullptr);
    REQUIRE(nodes.registry.references(physical).size() == 1);
    REQUIRE(Resource::destroyed == 0);

    nodes.ops.destroyBranch(callerB);
    REQUIRE(nodes.registry.get(physical) == nullptr);
    REQUIRE(Resource::destroyed == 1);
}

TEST(Mount_AddShareDiesWithPhysicalOwner, RuntimeFixture,
    "Уничтожение physical owner удаляет shared resource и все reference-ноды.")
{
    registerTypes(fixture);
    resetCounters();
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = createOwner(fixture);
    const NodeId callerA = nodes.factory.folder(fixture.root, "caller-a");
    const NodeId callerB = nodes.factory.folder(fixture.root, "caller-b");

    fixture.build(callerA).mount<SharedOwner>().addShare<Resource>("mesh", ResourceDesc{1});
    fixture.build(callerB).mount<SharedOwner>().addShare<Resource>("mesh", ResourceDesc{2});
    const auto api = fixture.run_ctx.blueprints.id<Resource>();
    const NodeId referenceA = nodes.registry.find("mesh", callerA, api, owner);
    const NodeId referenceB = nodes.registry.find("mesh", callerB, api, owner);

    nodes.ops.destroyBranch(owner);

    REQUIRE(nodes.registry.get(referenceA) == nullptr);
    REQUIRE(nodes.registry.get(referenceB) == nullptr);
    REQUIRE(Resource::destroyed == 1);
}

TEST(Mount_DescriptorlessAndRollback, RuntimeFixture,
    "Descriptorless overload работает, а исключение конструктора полностью откатывает resource.")
{
    registerTypes(fixture);
    resetCounters();
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = createOwner(fixture);
    const NodeId caller = nodes.factory.folder(fixture.root, "caller");
    auto mounted = fixture.build(caller).mount<SharedOwner>();

    REQUIRE(mounted.addLocal<PlainResource>("plain").exists());

    bool localRejected = false;
    try {
        mounted.addLocal<ThrowingResource>("bad-local", ResourceDesc{});
    } catch (const std::runtime_error&) {
        localRejected = true;
    }
    REQUIRE(localRejected);

    bool sharedRejected = false;
    try {
        mounted.addShare<ThrowingResource>("bad-shared", ResourceDesc{});
    } catch (const std::runtime_error&) {
        sharedRejected = true;
    }
    REQUIRE(sharedRejected);

    const auto throwing = fixture.run_ctx.blueprints.id<ThrowingResource>();
    REQUIRE(nodes.registry.find("bad-local", caller, throwing, owner) == InvalidNodeId);
    REQUIRE(nodes.registry.find("bad-shared", caller, throwing, owner) == InvalidNodeId);
    for (NodeId child : nodes.registry.children(owner)) {
        const auto& node = nodes.registry.require(child);
        REQUIRE(node.name != "bad-local");
        REQUIRE(node.name != "bad-shared");
    }
}

}
