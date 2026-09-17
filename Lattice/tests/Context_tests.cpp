#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>


namespace Lattice {

struct ContextDummy {};

TEST(Context_Find, RuntimeFixture,
    "find должен возвращать объект активного слота.")
{
    const ContextId slot = fixture.run_ctx.contexts.create({
        .name = "print"
    });

    REQUIRE(!Objects::valid(fixture.run_ctx.find("print")));

    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>("first");

    const auto dummy = fixture.root.find<ContextDummy>("first");
    REQUIRE(dummy.node);

    fixture.run_ctx.activate(slot, dummy.node->getId());

    REQUIRE(fixture.run_ctx.find("print") == dummy.node->getId());
}

TEST(Context_FindUnknown, RuntimeFixture,
    "find неизвестного имени должен возвращать InvalidObjectId.")
{
    REQUIRE(!Objects::valid(fixture.run_ctx.find("print")));
}

TEST(Context_Get, RuntimeFixture,
    "get должен возвращать объект слота по ContextId.")
{
    const ContextId slot = fixture.run_ctx.contexts.create({
        .name = "focus"
    });

    REQUIRE(!Objects::valid(fixture.run_ctx.get(slot)));
}

TEST(Context_GetInvalid, RuntimeFixture,
    "get неизвестного слота должен возвращать InvalidObjectId.")
{
    REQUIRE(!Objects::valid(fixture.run_ctx.get(99)));
}

TEST(Context_Activate, RuntimeFixture,
    "activate должен привязать объект к слоту.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>("first");

    const auto dummy = fixture.root.find<ContextDummy>("first");
    REQUIRE(dummy.node);

    const ContextId slot = fixture.run_ctx.contexts.create({
        .name = "focus"
    });

    fixture.run_ctx.activate(slot, dummy.node->getId());

    REQUIRE(fixture.run_ctx.get(slot) == dummy.node->getId());
    REQUIRE(fixture.run_ctx.find("focus") == dummy.node->getId());
}

TEST(Context_ActivateOverwrite, RuntimeFixture,
    "activate должен заменять текущий объект слота.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>("first");
    fixture.root.add<ContextDummy>("second");

    const auto first = fixture.root.find<ContextDummy>("first");
    const auto second = fixture.root.find<ContextDummy>("second");

    REQUIRE(first.node);
    REQUIRE(second.node);

    const ContextId slot = fixture.run_ctx.contexts.find("ContextDummy");
    REQUIRE(slot != InvalidContextId);
    REQUIRE(fixture.run_ctx.get(slot) == first.node->getId());

    fixture.run_ctx.activate(slot, second.node->getId());

    REQUIRE(fixture.run_ctx.get(slot) == second.node->getId());
    REQUIRE(fixture.run_ctx.find("ContextDummy") == second.node->getId());
}

TEST(Context_ActivateUnknownObject, RuntimeFixture,
    "activate неизвестного объекта должен бросать исключение.")
{
    const ContextId slot = fixture.run_ctx.contexts.create({
        .name = "print"
    });

    bool thrown = false;

    try {
        fixture.run_ctx.activate(slot, 999);
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Context_ActivateUnknownSlot, RuntimeFixture,
    "activate неизвестного слота не должен менять контекст.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>();

    const auto dummy = fixture.root.find<ContextDummy>();
    REQUIRE(dummy.node);

    fixture.run_ctx.activate(99, dummy.node->getId());

    REQUIRE(!Objects::valid(fixture.run_ctx.get(99)));
}

TEST(Context_Clear, RuntimeFixture,
    "clear должен удалить все слоты.")
{
    fixture.run_ctx.contexts.create({
        .name = "print"
    });

    REQUIRE(fixture.run_ctx.contexts.find("print") != InvalidContextId);

    fixture.run_ctx.clear();

    REQUIRE(fixture.run_ctx.contexts.find("print") == InvalidContextId);
}


struct NsChild {
    float size = 1.f;

    void configure(Node& n) {
        n.bind("size", &size);
    }
};

struct NsHost : ServiceAPI {
    explicit NsHost(Node&) {}

    void configure(Node& n) {
        n.on("nested", [] {});
    }

    void run() override {}
};

struct NsModel : Model {
    float dt = 0.01f;

    explicit NsModel(Node& n) {
        Node& data = n.addFolder("data");
        data.add<NsChild>("child");
        n.add<NsHost>("nested");
    }

    void configure(Node& n) {
        n.bind("dt", &dt);
    }

    void run() override {}
};

struct NsIO : ServiceAPI {
    explicit NsIO(Node&) {}

    void configure(Node& n) {
        n.on("load", [] {});
    }

    void run() override {}
};

static void registerNamespaces(RuntimeFixture& fixture) {
    fixture.blueprints.blueprint<NsChild>();
    fixture.blueprints.blueprint<NsHost, ServiceAPI>();
    fixture.blueprints.blueprint<NsModel, Model>();
    fixture.blueprints.blueprint<NsIO, ServiceAPI>();
}

TEST(Context_NamespaceRoots, RuntimeFixture,
    "Верхние компоненты и вложенные сервисы — корни неймспейсов, обычные дети нет.")
{
    registerNamespaces(fixture);

    fixture.root.add<NsIO>("io");
    fixture.root.add<NsModel>("u1");

    const auto io = fixture.root.find<NsIO>("io");
    const auto u1 = fixture.root.find<NsModel>("u1");
    const auto nested = u1.node->find<NsHost>("nested");
    Node* child = fixture.run_ctx.objects.require(u1.node->exported("NsChild")).node;

    REQUIRE(io.node);
    REQUIRE(u1.node);
    REQUIRE(child);
    REQUIRE(nested.node);

    REQUIRE(io.node->isNamespaceRoot());
    REQUIRE(u1.node->isNamespaceRoot());
    REQUIRE(!child->isNamespaceRoot());
    REQUIRE(nested.node->isNamespaceRoot());
    REQUIRE(child->nearestNamespaceRoot() == u1.node->getId());
    REQUIRE(nested.node->nearestNamespaceRoot() == nested.node->getId());
}

TEST(Context_NamespaceSwitchExportsChildren, RuntimeFixture,
    "Переключение модели должно сменить её bind и обычных детей, включая тех что в папке.")
{
    registerNamespaces(fixture);

    fixture.root.add<NsIO>("io");
    fixture.root.add<NsModel>("u1");
    fixture.root.add<NsModel>("u2");
    fixture.root.configureBranch();

    const auto io = fixture.root.find<NsIO>("io");
    const auto u1 = fixture.root.find<NsModel>("u1");
    const auto u2 = fixture.root.find<NsModel>("u2");
    Node* child1 = fixture.run_ctx.objects.require(u1.node->exported("NsChild")).node;
    Node* child2 = fixture.run_ctx.objects.require(u2.node->exported("NsChild")).node;
    const auto nested1 = u1.node->find<NsHost>("nested");

    REQUIRE(fixture.run_ctx.find("NsModel") == u1.node->getId());
    REQUIRE(fixture.run_ctx.find("dt") == u1.node->exported("dt"));
    REQUIRE(fixture.run_ctx.find("size") == child1->exported("size"));
    REQUIRE(fixture.run_ctx.find("NsChild") == child1->getId());
    REQUIRE(fixture.run_ctx.find("load") == io.node->exported("load"));
    REQUIRE(fixture.run_ctx.find("NsHost") == nested1.node->getId());

    REQUIRE(u2.node->exported("dt") != fixture.run_ctx.find("dt"));
    REQUIRE(u2.node->exported("size") == child2->exported("size"));

    u2.node->activateNamespace();

    REQUIRE(fixture.run_ctx.find("NsModel") == u2.node->getId());
    REQUIRE(fixture.run_ctx.find("dt") == u2.node->exported("dt"));
    REQUIRE(fixture.run_ctx.find("size") == child2->exported("size"));
    REQUIRE(fixture.run_ctx.find("NsChild") == child2->getId());
    REQUIRE(fixture.run_ctx.namespaceOf("dt") == u2.node->getId());
    REQUIRE(fixture.run_ctx.namespaceOf("size") == u2.node->getId());
}

TEST(Context_NamespaceSwitchSkipsSiblingAndNestedService, RuntimeFixture,
    "Соседний неймспейс и вложенный Service не должны уезжать вместе с моделью.")
{
    registerNamespaces(fixture);

    fixture.root.add<NsIO>("io");
    fixture.root.add<NsModel>("u1");
    fixture.root.add<NsModel>("u2");
    fixture.root.configureBranch();

    const auto io = fixture.root.find<NsIO>("io");
    const auto u1 = fixture.root.find<NsModel>("u1");
    const auto u2 = fixture.root.find<NsModel>("u2");
    const auto nested1 = u1.node->find<NsHost>("nested");
    const auto nested2 = u2.node->find<NsHost>("nested");

    const ObjectId load = fixture.run_ctx.find("load");
    const ObjectId host = fixture.run_ctx.find("NsHost");
    const ObjectId nestedAction = fixture.run_ctx.find("nested");

    REQUIRE(load == io.node->exported("load"));
    REQUIRE(host == nested1.node->getId());
    REQUIRE(nestedAction == nested1.node->exported("nested"));
    REQUIRE(nested2.node->exported("nested") != nestedAction);

    u2.node->activateNamespace();

    REQUIRE(fixture.run_ctx.find("load") == load);
    REQUIRE(fixture.run_ctx.namespaceOf("load") == io.node->getId());
    REQUIRE(fixture.run_ctx.find("NsHost") == nested1.node->getId());
    REQUIRE(fixture.run_ctx.find("nested") == nestedAction);
    REQUIRE(fixture.run_ctx.find("NsModel") == u2.node->getId());
}

} // namespace Lattice
