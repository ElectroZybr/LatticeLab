#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>


namespace Lattice {

struct SlotAPI {
    virtual ~SlotAPI() = default;
    virtual int id() const = 0;
};

struct SlotImplA : SlotAPI {
    explicit SlotImplA(Node&) {}
    int id() const override { return 1; }
    bool configured = false;
    void configure(Node&) { configured = true; }
};

struct SlotImplB : SlotAPI {
    explicit SlotImplB(Node&) {}
    int id() const override { return 2; }
    void configure(Node&) {}
};

struct SlotInput {
    bool configured = false;
    void configure(Node&) { configured = true; }
};

struct SlotWindowImpl : SlotAPI {
    explicit SlotWindowImpl(Node& branch) {
        branch.add<SlotInput>();
    }
    void configure(Node&) {}
    int id() const override { return 7; }
};

struct SlotRender {
    int configures = 0;
    Slot<SlotAPI> window;
    void configure(Node& branch) {
        ++configures;
        window = branch.find<SlotAPI>();
    }
};

struct SlotHost {
    explicit SlotHost(Node& branch) {
        branch.slot<SlotAPI>();
        branch.add<SlotRender>();
    }

    void configure(Node& branch) {
        branch.find<SlotAPI>().use("SlotWindowImpl");
    }
};

struct SlotNeighbor {
    int configures = 0;
    SlotAPI* api = nullptr;
    void configure(Node& branch) {
        ++configures;
        api = branch.find<SlotAPI>().get();
    }
};

static void registerSlotTypes(RuntimeFixture& fixture) {
    fixture.blueprints.blueprint<SlotAPI>();
    fixture.blueprints.blueprint<SlotImplA, SlotAPI>();
    fixture.blueprints.blueprint<SlotImplB, SlotAPI>();
    fixture.blueprints.blueprint<SlotInput>();
    fixture.blueprints.blueprint<SlotWindowImpl, SlotAPI>();
    fixture.blueprints.blueprint<SlotRender>();
    fixture.blueprints.blueprint<SlotHost>();
    fixture.blueprints.blueprint<SlotNeighbor>();
}

TEST(Slot_EmptyNode, RuntimeFixture,
    "Пустой слот: нода есть, object == nullptr.")
{
    registerSlotTypes(fixture);

    auto slot = fixture.root.slot<SlotAPI>();

    REQUIRE(slot.node);
    REQUIRE(slot.node->name().empty());
    REQUIRE(slot.node->getKind() == NodeKind::Slot);
    REQUIRE(slot.node->getObject() == nullptr);
    REQUIRE(!slot.exists());
    REQUIRE(fixture.root.find<SlotAPI>().node == slot.node);
}

TEST(Slot_RequireEmptyThrows, RuntimeFixture,
    "require на пустом слоте бросает, find возвращает ноду.")
{
    registerSlotTypes(fixture);
    fixture.root.slot<SlotAPI>();

    REQUIRE(fixture.root.find<SlotAPI>().node);
    REQUIRE(!fixture.root.find<SlotAPI>().exists());

    bool thrown = false;
    try {
        fixture.root.require<SlotAPI>();
    } catch (const Exception&) {
        thrown = true;
    }
    REQUIRE(thrown);
}

TEST(Slot_UseFillsSameNode, RuntimeFixture,
    "use(impl) заполняет ту же ноду: стабильный id, без дочернего импла.")
{
    registerSlotTypes(fixture);

    auto slot = fixture.root.slot<SlotAPI>();
    const ObjectId id = slot.node->getId();

    slot.use("SlotImplA");

    REQUIRE(slot.node->getId() == id);
    REQUIRE(slot.exists());
    REQUIRE(slot->id() == 1);
    REQUIRE(slot.node->getKind() == NodeKind::Slot);
    REQUIRE(slot.node->directCollect<SlotImplA>().empty());
    REQUIRE(fixture.root.require<SlotAPI>().getPtr() == slot.get());
}

TEST(Slot_UseReplacesImpl, RuntimeFixture,
    "Повторный use сносит детей слота и ставит новый импл в ту же ноду.")
{
    registerSlotTypes(fixture);

    auto slot = fixture.root.slot<SlotAPI>();
    const ObjectId id = slot.node->getId();

    slot.use("SlotWindowImpl");
    REQUIRE(slot.node->directCollect<SlotInput>().size() == 1);

    slot.use("SlotImplB");

    REQUIRE(slot.node->getId() == id);
    REQUIRE(slot->id() == 2);
    REQUIRE(slot.node->directCollect<SlotInput>().empty());
}

TEST(Slot_RequireWalksFloorNotContext, RuntimeFixture,
    "require ищет слот на этаже (дети, затем дети предков), не через Context.")
{
    registerSlotTypes(fixture);

    fixture.root.slot<SlotAPI>();
    fixture.root.find<SlotAPI>().use("SlotImplA");

    Node& branch = fixture.root.addFolder("branch");

    REQUIRE(!Objects::valid(fixture.run_ctx.find("SlotAPI")));
    REQUIRE(branch.require<SlotAPI>()->id() == 1);
}

TEST(Slot_FloorDoesNotCrossSiblings, RuntimeFixture,
    "Поиск на этаже не заходит в соседние ветки.")
{
    registerSlotTypes(fixture);

    Node& a = fixture.root.addFolder("A");
    a.slot<SlotAPI>();
    a.find<SlotAPI>().use("SlotImplA");

    Node& b = fixture.root.addFolder("B");

    REQUIRE(a.find<SlotAPI>().exists());
    REQUIRE(!b.find<SlotAPI>().node);
}

TEST(Slot_TreeOwnership, RuntimeFixture,
    "Render — сосед слота, инпуты — дети импла, не наоборот.")
{
    registerSlotTypes(fixture);

    fixture.root.add<SlotHost>();
    fixture.root.configureAll();

    auto host = fixture.root.find<SlotHost>();
    auto window = host.node->find<SlotAPI>();
    auto render = host.node->find<SlotRender>();

    REQUIRE(window.exists());
    REQUIRE(window->id() == 7);
    REQUIRE(window.node->getParent() == host.node);
    REQUIRE(render.node->getParent() == host.node);
    REQUIRE(window.node->directCollect<SlotInput>().size() == 1);
    REQUIRE(window.node->directCollect<SlotRender>().empty());
    REQUIRE(render->window.get() == window.get());
    REQUIRE(window.node->directCollect<SlotInput>()[0]->configured);
}

TEST(Slot_LateUseReconfiguresFloor, RuntimeFixture,
    "После позднего use повторный configure только у сиблингов этажа слота.")
{
    registerSlotTypes(fixture);

    fixture.root.slot<SlotAPI>();
    fixture.root.add<SlotNeighbor>();
    fixture.root.configureAll();

    auto neighbor = fixture.root.require<SlotNeighbor>();
    REQUIRE(neighbor->configures == 1);
    REQUIRE(neighbor->api == nullptr);

    const ObjectId id = fixture.root.find<SlotAPI>().node->getId();
    fixture.root.find<SlotAPI>().use("SlotImplA");

    REQUIRE(fixture.root.find<SlotAPI>().node->getId() == id);
    REQUIRE(neighbor->configures == 2);
    REQUIRE(neighbor->api);
    REQUIRE(neighbor->api->id() == 1);
}

TEST(Slot_CollectFromFloor, RuntimeFixture,
    "collect собирает API с этажа (потомки родителя), не каждый тик.")
{
    registerSlotTypes(fixture);

    fixture.root.add<SlotHost>();
    Node& map = fixture.root.addFolder("ActionMap");
    fixture.root.configureAll();

    auto inputs = map.collect<SlotInput>();
    REQUIRE(inputs.size() == 1);
}

TEST(Slot_UseFromParent, RuntimeFixture,
    "use<API, Impl>() с родителя заполняет слот-ребёнка.")
{
    registerSlotTypes(fixture);

    fixture.root.slot<SlotAPI>();
    fixture.root.use<SlotAPI, SlotImplA>();

    REQUIRE(fixture.root.require<SlotAPI>()->id() == 1);
}

} // namespace Lattice
