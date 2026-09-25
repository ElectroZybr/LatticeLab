#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>

namespace Lattice {

struct SlotAPI {
    virtual ~SlotAPI() = default;
    virtual int id() const = 0;
};

struct SlotImplA : SlotAPI {
    bool configured = false;

    explicit SlotImplA(NodeBuild) {}
    int id() const override { return 1; }

    void configure(NodeConfigure) {
        configured = true;
    }
};

struct SlotImplB : SlotAPI {
    explicit SlotImplB(NodeBuild) {}
    int id() const override { return 2; }
    void configure(NodeConfigure) {}
};

struct SlotInput {
    bool configured = false;

    explicit SlotInput(NodeBuild) {}

    void configure(NodeConfigure) {
        configured = true;
    }
};

struct SlotWindowImpl : SlotAPI {
    explicit SlotWindowImpl(NodeBuild node) {
        node.add<SlotInput>();
    }

    void configure(NodeConfigure) {}

    int id() const override {
        return 7;
    }
};

struct SlotRender {
    int configures = 0;
    Slot<SlotAPI> window;

    explicit SlotRender(NodeBuild node)
        : window(node.addSlot<SlotAPI>()) {}

    void configure(NodeConfigure) {
        ++configures;
    }
};

struct SlotHost {
    explicit SlotHost(NodeBuild node) {
        node.addSlot<SlotAPI>();
        node.add<SlotRender>();
    }

    void configure(NodeConfigure node) {
        node.requireSlot<SlotAPI>().choice<SlotWindowImpl>();
    }
};

struct SlotNeighbor {
    int configures = 0;
    SlotAPI* api = nullptr;

    explicit SlotNeighbor(NodeBuild) {}

    void configure(NodeConfigure node) {
        ++configures;
        api = node.find<SlotAPI>().get();
    }
};

static void registerSlotTypes(RuntimeFixture& fixture) {
    BlueprintRegister::add<SlotAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotImplA, SlotAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotImplB, SlotAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotInput>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotWindowImpl, SlotAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotRender>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotHost>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<SlotNeighbor>(fixture.run_ctx.blueprints);
}


TEST(Slot_EmptyNode, RuntimeFixture,
    "Пустой слот должен существовать как нода без runtime object.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    Slot<SlotAPI> slot{
        fixture.run_ctx.nodes.factory,
        fixture.run_ctx.nodes.query,
        fixture.run_ctx.blueprints,
        id
    };

    const auto& node = fixture.run_ctx.nodes.registry.require(id);

    REQUIRE(node.kind == NodeKind::Slot);
    REQUIRE(node.bp == fixture.run_ctx.blueprints.id<SlotAPI>());
    REQUIRE(node.object.ptr == nullptr);
    REQUIRE(node.object.bp == InvalidBlueprintId);
    REQUIRE(!slot.exists());
}

TEST(Slot_ChoiceReplacesImpl, RuntimeFixture,
    "Повторный choice должен уничтожить старое содержимое slot и поставить новую реализацию.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    Slot<SlotAPI> slot{
        fixture.run_ctx.nodes.factory,
        fixture.run_ctx.nodes.query,
        fixture.run_ctx.blueprints,
        id
    };

    slot.choice<SlotWindowImpl>();

    REQUIRE(slot->id() == 7);
    REQUIRE(fixture.run_ctx.nodes.query.collect(id, fixture.run_ctx.blueprints.id<SlotInput>()).size() == 1);

    slot.choice<SlotImplB>();

    REQUIRE(slot.id() == id);
    REQUIRE(slot->id() == 2);
    REQUIRE(fixture.run_ctx.nodes.query.collect(id, fixture.run_ctx.blueprints.id<SlotInput>()).empty());
}

TEST(Slot_RejectsWrongImpl, RuntimeFixture,
    "Slot не должен принимать blueprint, который не реализует его API.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    Slot<SlotAPI> slot{
        fixture.run_ctx.nodes.factory,
        fixture.run_ctx.nodes.query,
        fixture.run_ctx.blueprints,
        id
    };

    bool thrown = false;

    try {
        slot.choice(fixture.run_ctx.blueprints.id<SlotInput>());
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(!slot.exists());
}

TEST(Slot_ChoiceSameImpl, RuntimeFixture,
    "Повторный выбор уже установленной реализации не должен пересоздавать объект.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    Slot<SlotAPI> slot{
        fixture.run_ctx.nodes.factory,
        fixture.run_ctx.nodes.query,
        fixture.run_ctx.blueprints,
        id
    };

    slot.choice<SlotImplA>();
    SlotAPI* first = slot.get();

    slot.choice<SlotImplA>();

    REQUIRE(slot.get() == first);
}

TEST(Slot_CreatesContextRole, RuntimeFixture,
    "Создание slot должно сразу зарегистрировать его как target соответствующей роли.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(fixture.root);
    const RoleId role = fixture.run_ctx.nodes.context.findRole(typeKey<SlotAPI>());

    REQUIRE(scope != InvalidContextScopeId);
    REQUIRE(role != InvalidRoleId);
    REQUIRE(fixture.run_ctx.nodes.context.resolve(scope, role) == id);
}

TEST(Slot_FocusTracksImplementation, RuntimeFixture,
    "Focus на роль slot должен видеть текущую реализацию после choice.")
{
    registerSlotTypes(fixture);

    const NodeId id = fixture.run_ctx.nodes.factory.slot(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotAPI>()
    );

    Slot<SlotAPI> slot{
        fixture.run_ctx.nodes.factory,
        fixture.run_ctx.nodes.query,
        fixture.run_ctx.blueprints,
        id
    };

    auto focus = fixture.run_ctx.nodes.configure(fixture.root).focus<SlotAPI>();

    REQUIRE(!focus);

    slot.choice<SlotImplA>();

    REQUIRE(focus);
    REQUIRE(focus.id() == id);
    REQUIRE(focus->id() == 1);

    slot.choice<SlotImplB>();

    REQUIRE(focus.id() == id);
    REQUIRE(focus->id() == 2);
}

namespace {
struct RollbackChild {
    static inline int alive = 0;
    RollbackChild() { ++alive; }
    ~RollbackChild() { --alive; }
};
struct ThrowingSlotImpl : SlotAPI {
    explicit ThrowingSlotImpl(NodeBuild node) {
        node.add<RollbackChild>();
        throw Exception("SlotTest", "Constructor failed");
    }
    int id() const override { return 3; }
};
template<class F> bool slotThrows(F&& operation) {
    try { operation(); } catch (const Exception&) { return true; }
    return false;
}
}

TEST(Slot_RequireEmptyAndRejectComponent, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;
    auto slot = nodes.build(fixture.root).addSlot<SlotAPI>();
    auto view = nodes.configure(fixture.root);
    REQUIRE(view.findSlot<SlotAPI>().has_value());
    auto required = view.requireSlot<SlotAPI>();
    REQUIRE(required.id() == slot.id());
    REQUIRE(!required);
    required.choice<SlotImplA>();
    REQUIRE(slot->id() == 1);
    nodes.build(fixture.root).add<SlotImplA>("component");
    REQUIRE(!view.findSlot<SlotAPI>("component"));
    REQUIRE(slotThrows([&] { view.requireSlot<SlotAPI>("component"); }));
    REQUIRE(!view.findSlot<SlotAPI>("missing"));
    REQUIRE(slotThrows([&] { view.requireSlot<SlotAPI>("missing"); }));
}

TEST(Slot_RepeatedCreation, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;
    auto first = nodes.build(fixture.root).addSlot<SlotAPI>();
    first.choice<SlotImplA>();
    auto again = nodes.build(fixture.root).addSlot<SlotAPI>();
    REQUIRE(first.id() == again.id());
    REQUIRE(first.get() == again.get());
    REQUIRE(nodes.registry.children(fixture.root).size() == 1);
}

TEST(Slot_AbstractChoicePreservesContents, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;
    auto slot = nodes.build(fixture.root).addSlot<SlotAPI>();
    slot.choice<SlotWindowImpl>();
    auto* previous = slot.get();
    REQUIRE(slotThrows([&] { slot.choice<SlotAPI>(); }));
    REQUIRE(slot.get() == previous);
    REQUIRE(nodes.registry.children(slot.id()).size() == 1);
}

TEST(Slot_ConstructorFailureClearsChildren, RuntimeFixture) {
    registerSlotTypes(fixture);
    BlueprintRegister::add<RollbackChild>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<ThrowingSlotImpl, SlotAPI>(fixture.run_ctx.blueprints);
    auto& nodes = fixture.run_ctx.nodes;
    auto slot = nodes.build(fixture.root).addSlot<SlotAPI>();
    slot.choice<SlotImplA>();
    auto focus = nodes.configure(fixture.root).focus<SlotAPI>();
    REQUIRE(slotThrows([&] { slot.choice<ThrowingSlotImpl>(); }));
    REQUIRE(!slot);
    REQUIRE(RollbackChild::alive == 0);
    REQUIRE(nodes.registry.children(slot.id()).empty());
    REQUIRE(focus.id() == slot.id());
    REQUIRE(!focus);
    slot.choice<SlotImplB>();
    REQUIRE(focus->id() == 2);
}

TEST(Node_RepeatedCreationAndUnnamedTypes, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;
    auto a = nodes.build(fixture.root).add<SlotImplA>();
    auto b = nodes.build(fixture.root).add<SlotImplB>();
    REQUIRE(a->id() == 1);
    REQUIRE(b->id() == 2);
    REQUIRE(nodes.build(fixture.root).add<SlotImplA>().get() == a.get());
}

TEST(Node_MountSharedCandidate, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;

    const auto api = fixture.run_ctx.blueprints.id<SlotAPI>();
    const auto impl = fixture.run_ctx.blueprints.id<SlotImplA>();
    const NodeId provider = nodes.factory.component(fixture.root, impl, "provider");
    nodes.factory.share(provider, api);

    const NodeId consumer = nodes.factory.folder(fixture.root, "consumer");
    auto mounted = nodes.build(consumer).mount<SlotAPI>();

    REQUIRE(mounted.target() == provider);
    REQUIRE(mounted.ref()->id() == 1);

    mounted.addLocal<SlotInput>("input");
    const NodeId reference = nodes.registry.find(
        "input",
        consumer,
        fixture.run_ctx.blueprints.id<SlotInput>(),
        provider
    );
    REQUIRE(reference != InvalidNodeId);
    REQUIRE(nodes.registry.require(reference).kind == NodeKind::Mount);
    REQUIRE(nodes.registry.require(nodes.registry.require(reference).relation).parent == provider);
}

TEST(Node_MountRejectsAmbiguousCandidates, RuntimeFixture) {
    registerSlotTypes(fixture);
    auto& nodes = fixture.run_ctx.nodes;

    const auto api = fixture.run_ctx.blueprints.id<SlotAPI>();
    const NodeId a = nodes.factory.component(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotImplA>(),
        "a"
    );
    const NodeId b = nodes.factory.component(
        fixture.root,
        fixture.run_ctx.blueprints.id<SlotImplB>(),
        "b"
    );

    nodes.factory.share(a, api);
    nodes.factory.share(b, api);

    const NodeId consumer = nodes.factory.folder(fixture.root, "consumer");
    REQUIRE(slotThrows([&] { nodes.build(consumer).mount<SlotAPI>(); }));
}

}
