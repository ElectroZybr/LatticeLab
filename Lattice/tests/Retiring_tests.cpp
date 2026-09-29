#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

struct RetiringTarget : Component {
    inline static bool ready = false;
    inline static size_t retireCalls = 0;
    inline static size_t destroyCalls = 0;

    explicit RetiringTarget(NodeBuild node) {
        node.action("ping", [] {});
    }

    void retire() {
        ++retireCalls;
    }

    bool readyToDestroy() const {
        return ready;
    }

    ~RetiringTarget() {
        ++destroyCalls;
    }
};

struct RetiringConsumer : Component {
    inline static size_t destroyCalls = 0;

    void configure(NodeConfigure node) {
        node.require<RetiringTarget>("target");
    }

    ~RetiringConsumer() {
        ++destroyCalls;
    }
};

struct ImmediatelyRetired : Component {
    inline static size_t destroyCalls = 0;

    ~ImmediatelyRetired() {
        ++destroyCalls;
    }
};

struct CollectingHost : ServiceAPI {
    void run() override {
        stopRequested();
    }
};

}

TEST(Retiring_HidesClosureAndWaitsForReadiness, RuntimeFixture,
    "Retiring должен сразу скрыть dependency closure, но сохранить объекты до readiness.") {
    auto& nodes = fixture.run_ctx.nodes;
    RetiringTarget::ready = false;
    RetiringTarget::retireCalls = 0;
    RetiringTarget::destroyCalls = 0;
    RetiringConsumer::destroyCalls = 0;

    const BlueprintId targetBlueprint = BlueprintRegister::add<RetiringTarget, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId consumerBlueprint = BlueprintRegister::add<RetiringConsumer, Component>(
        fixture.run_ctx.blueprints
    );

    const NodeId target = nodes.builder.add(fixture.root, targetBlueprint, "target");
    const NodeId consumer = nodes.builder.add(fixture.root, consumerBlueprint, "consumer");
    REQUIRE(nodes.exports.find(target, "ping") != InvalidExportId);

    nodes.ops.retireBranch(target);
    nodes.ops.retireBranch(target);

    REQUIRE(nodes.registry.require(target).state == NodeState::Retiring);
    REQUIRE(nodes.registry.require(consumer).state == NodeState::Retiring);
    REQUIRE(nodes.query.find(fixture.root, targetBlueprint, "target") == InvalidNodeId);
    REQUIRE(nodes.query.find(fixture.root, consumerBlueprint, "consumer") == InvalidNodeId);
    REQUIRE(nodes.query.resolve(target, targetBlueprint) == nullptr);
    REQUIRE(nodes.exports.find(target, "ping") == InvalidExportId);
    REQUIRE(RetiringTarget::retireCalls == 1);
    REQUIRE(nodes.ops.collectRetired() == 0);
    REQUIRE(nodes.registry.get(target) != nullptr);
    REQUIRE(nodes.registry.get(consumer) != nullptr);
    REQUIRE(RetiringTarget::destroyCalls == 0);
    REQUIRE(RetiringConsumer::destroyCalls == 0);

    RetiringTarget::ready = true;
    REQUIRE(nodes.ops.collectRetired() > 0);
    REQUIRE(nodes.registry.get(target) == nullptr);
    REQUIRE(nodes.registry.get(consumer) == nullptr);
    REQUIRE(RetiringTarget::destroyCalls == 1);
    REQUIRE(RetiringConsumer::destroyCalls == 1);
}

TEST(Retiring_ComponentWithoutLifecycleIsCollectedImmediately, RuntimeFixture,
    "Компонент без lifecycle hooks должен быть готов к физическому удалению сразу.") {
    auto& nodes = fixture.run_ctx.nodes;
    ImmediatelyRetired::destroyCalls = 0;

    const BlueprintId blueprint = BlueprintRegister::add<ImmediatelyRetired, Component>(
        fixture.run_ctx.blueprints
    );
    const NodeId node = nodes.builder.add(fixture.root, blueprint, "immediate");

    REQUIRE(fixture.run_ctx.blueprints.require(blueprint).meta.ops != nullptr);
    REQUIRE(fixture.run_ctx.blueprints.require(blueprint).meta.ops->retire == nullptr);
    REQUIRE(fixture.run_ctx.blueprints.require(blueprint).meta.ops->readyToDestroy == nullptr);

    nodes.ops.retireBranch(node);
    REQUIRE(nodes.registry.get(node) != nullptr);
    REQUIRE(nodes.ops.collectRetired() > 0);
    REQUIRE(nodes.registry.get(node) == nullptr);
    REQUIRE(ImmediatelyRetired::destroyCalls == 1);
}

TEST(Retiring_HostServiceCollectsAtCheckpoint, RuntimeFixture,
    "Host ServiceAPI должен обслуживать retiring queue без участия реализации сервиса.") {
    auto& nodes = fixture.run_ctx.nodes;
    ImmediatelyRetired::destroyCalls = 0;

    const BlueprintId victimBlueprint = BlueprintRegister::add<ImmediatelyRetired, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId hostBlueprint = BlueprintRegister::add<CollectingHost, ServiceAPI>(
        fixture.run_ctx.blueprints
    );
    const NodeId victim = nodes.builder.add(fixture.root, victimBlueprint, "victim");
    const NodeId host = nodes.builder.add(fixture.root, hostBlueprint, "host");

    nodes.ops.retireBranch(victim);
    REQUIRE(nodes.registry.get(victim) != nullptr);

    auto* service = static_cast<ServiceAPI*>(
        nodes.query.resolve(host, fixture.run_ctx.blueprints.id<ServiceAPI>())
    );
    REQUIRE(service != nullptr);
    service->enter(nodes.ops);

    REQUIRE(nodes.registry.get(victim) == nullptr);
    REQUIRE(ImmediatelyRetired::destroyCalls == 1);
}

TEST(Retiring_ChildrenDelKeepsObjectQueuedUntilReady, RuntimeFixture,
    "Children<T>::del должен убрать ребёнка из handler до физического удаления.") {
    auto& nodes = fixture.run_ctx.nodes;
    RetiringTarget::ready = false;
    RetiringTarget::retireCalls = 0;
    RetiringTarget::destroyCalls = 0;
    BlueprintRegister::add<RetiringTarget, Component>(fixture.run_ctx.blueprints);
    auto children = nodes.configure(fixture.root).children<RetiringTarget>();
    const NodeId child = children.add("queued");

    children.del(child);

    REQUIRE(children.empty());
    REQUIRE(nodes.registry.get(child) != nullptr);
    REQUIRE(nodes.registry.require(child).state == NodeState::Retiring);
    REQUIRE(RetiringTarget::retireCalls == 1);
    REQUIRE(RetiringTarget::destroyCalls == 0);

    RetiringTarget::ready = true;
    REQUIRE(nodes.ops.collectRetired() == 1);
    REQUIRE(nodes.registry.get(child) == nullptr);
    REQUIRE(RetiringTarget::destroyCalls == 1);
}

}
