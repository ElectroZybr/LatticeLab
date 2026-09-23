#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>


namespace Lattice {

class TestComponent {
public:
    int value = 0;
    bool configured = false;

    void configure(NodeConfigureView) {
        configured = true;
    }
};

class TestAPI {
public:
    virtual ~TestAPI() = default;
};

class TestImplA : public TestAPI {
public:
    int value = 10;
};

class TestImplB : public TestAPI {
public:
    int value = 20;
};


TEST(Node_DeepTreeLookup, RuntimeFixture, 
"Поиск компонента должен подниматься по дереву родителей, но не заходить в соседние ветки. \
Child-ветка должна видеть свои компоненты и компоненты предков.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branchA = nodes.factory.folder(fixture.root, "BranchA");
    nodes.build(branchA).add<TestComponent>("a");

    const NodeId branchB = nodes.factory.folder(fixture.root, "BranchB");
    nodes.build(branchB).add<TestComponent>("b");

    const NodeId branchAChild = nodes.factory.folder(branchA, "BranchAChild");
    nodes.build(branchAChild).add<TestComponent>("child");

    REQUIRE(nodes.configure(fixture.root).find<TestComponent>("root").exists());
    REQUIRE(nodes.configure(branchA).find<TestComponent>("root").exists());
    REQUIRE(nodes.configure(branchAChild).find<TestComponent>("root").exists());

    REQUIRE(nodes.configure(branchA).find<TestComponent>("a").exists());
    REQUIRE(nodes.configure(branchAChild).find<TestComponent>("a").exists());

    REQUIRE(nodes.configure(branchB).find<TestComponent>("b").exists());
    REQUIRE(!nodes.configure(branchA).find<TestComponent>("b").exists());
    REQUIRE(!nodes.configure(fixture.root).find<TestComponent>("missing").exists());
}

TEST(Node_TreeShadowing, RuntimeFixture, 
"Компонент в дочерней ветке должен скрывать компонент с тем же именем из родительской ветки. \
При этом оба объекта должны оставаться независимыми экземплярами.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("shared");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("shared");

    auto rootComponent = nodes.configure(fixture.root).require<TestComponent>("shared");
    auto branchComponent = nodes.configure(branch).require<TestComponent>("shared");

    REQUIRE(rootComponent);
    REQUIRE(branchComponent);
    REQUIRE(rootComponent.get() != branchComponent.get());
}

TEST(Node_ShadowingDoesNotLeak, RuntimeFixture, 
"Одинаковые имена компонентов в соседних ветках не должны влиять друг на друга. \
Поиск из одной ветки не должен случайно находить локальный компонент другой ветки.")
 {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("shared");

    const NodeId branchA = nodes.factory.folder(fixture.root, "A");
    const NodeId branchB = nodes.factory.folder(fixture.root, "B");

    nodes.build(branchA).add<TestComponent>("shared");

    auto a = nodes.configure(branchA).find<TestComponent>("shared");
    auto b = nodes.configure(branchB).find<TestComponent>("shared");

    REQUIRE(a.exists());
    REQUIRE(b.exists());
    REQUIRE(a.get() != b.get());
}

TEST(Node_TreeFolderCollect, RuntimeFixture,
    "Поиск в папке должен возвращать компоненты из текущей папки и всех вложенных папок.") {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("a");
    nodes.build(branch).add<TestComponent>("b");

    const NodeId child = nodes.factory.folder(branch, "child");
    nodes.build(child).add<TestComponent>("c");

    const NodeId nested = nodes.factory.folder(child, "nested");
    nodes.build(nested).add<TestComponent>("d");

    auto root = nodes.query.collect(fixture.root, typeKey<TestComponent>());
    auto branchNode = nodes.query.collect(branch, typeKey<TestComponent>());
    auto childNode = nodes.query.collect(child, typeKey<TestComponent>());
    auto nestedNode = nodes.query.collect(nested, typeKey<TestComponent>());

    REQUIRE(root.size() == 5);
    REQUIRE(branchNode.size() == 4);
    REQUIRE(childNode.size() == 2);
    REQUIRE(nestedNode.size() == 1);
}


TEST(Node_DirectChildren, RuntimeFixture,
    "children возвращает непосредственных детей, включая папки.") {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("a");
    nodes.build(branch).add<TestComponent>("b");

    const NodeId child = nodes.factory.folder(branch, "child");
    nodes.build(child).add<TestComponent>("c");

    const NodeId nested = nodes.factory.folder(child, "nested");
    nodes.build(nested).add<TestComponent>("d");

    auto root = nodes.registry.children(fixture.root);
    auto branchNode = nodes.registry.children(branch);
    auto childNode = nodes.registry.children(child);
    auto nestedNode = nodes.registry.children(nested);

    REQUIRE(root.size() == 2);
    REQUIRE(branchNode.size() == 3);
    REQUIRE(childNode.size() == 2);
    REQUIRE(nestedNode.size() == 1);
}

TEST(Node_AddImpls, RuntimeFixture,
    "addImpls возвращает реализации API; collect различает конкретные типы.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<TestImplA, TestAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<TestImplB, TestAPI>(fixture.run_ctx.blueprints);

    auto impls = nodes.build(fixture.root).addImpls<TestAPI>();

    auto exactA = nodes.query.collect(fixture.root, typeKey<TestImplA>());
    auto nested = nodes.registry.children(nodes.factory.folder(fixture.root, "nested"));

    REQUIRE(impls.size() == 2);
    REQUIRE(exactA.size() == 1);
    REQUIRE(nested.empty());
}

TEST(Node_GlobalCollectDeepTree, RuntimeFixture,
    "globalCollect должен найти каждый компонент во всём дереве независимо от глубины и ветки.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branchA = nodes.factory.folder(fixture.root, "A");
    nodes.build(branchA).add<TestComponent>("a");

    const NodeId branchB = nodes.factory.folder(fixture.root, "B");
    nodes.build(branchB).add<TestComponent>("b");

    const NodeId childA = nodes.factory.folder(branchA, "ChildA");
    nodes.build(childA).add<TestComponent>("aa");

    const NodeId childB = nodes.factory.folder(branchB, "ChildB");
    nodes.build(childB).add<TestComponent>("bb");

    const NodeId deep = nodes.factory.folder(childA, "Deep");
    nodes.build(deep).add<TestComponent>("aaa");

    auto rootComponent = nodes.configure(fixture.root).require<TestComponent>("root");
    auto a = nodes.configure(branchA).require<TestComponent>("a");
    auto b = nodes.configure(branchB).require<TestComponent>("b");
    auto aa = nodes.configure(childA).require<TestComponent>("aa");
    auto bb = nodes.configure(childB).require<TestComponent>("bb");
    auto aaa = nodes.configure(deep).require<TestComponent>("aaa");

    auto result = nodes.query.collect(nodes.ops.root(deep), typeKey<TestComponent>());

    REQUIRE(result.size() == 6);

    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == rootComponent.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == a.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == b.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == aa.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == bb.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == aaa.get(); }));
}

TEST(Node_GlobalCollectDifferentInstances, RuntimeFixture,
    "globalCollect должен возвращать все экземпляры одного типа независимо от их имён.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("one");
    nodes.build(fixture.root).add<TestComponent>("two");
    nodes.build(fixture.root).add<TestComponent>("three");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");

    nodes.build(branch).add<TestComponent>("four");
    nodes.build(branch).add<TestComponent>("five");

    auto one = nodes.configure(fixture.root).require<TestComponent>("one");
    auto two = nodes.configure(fixture.root).require<TestComponent>("two");
    auto three = nodes.configure(fixture.root).require<TestComponent>("three");
    auto four = nodes.configure(branch).require<TestComponent>("four");
    auto five = nodes.configure(branch).require<TestComponent>("five");

    auto result = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<TestComponent>());

    REQUIRE(result.size() == 5);

    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == one.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == two.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == three.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == four.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == five.get(); }));
}

TEST(Node_GlobalCollectSameNames, RuntimeFixture,
    "globalCollect должен различать объекты с одинаковыми именами в разных ветках.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    const NodeId branchA = nodes.factory.folder(fixture.root, "A");
    const NodeId branchB = nodes.factory.folder(fixture.root, "B");

    nodes.build(branchA).add<TestComponent>("shared");
    nodes.build(branchB).add<TestComponent>("shared");

    auto a = nodes.configure(branchA).require<TestComponent>("shared");
    auto b = nodes.configure(branchB).require<TestComponent>("shared");

    auto result = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<TestComponent>());

    REQUIRE(result.size() == 2);

    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == a.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == b.get(); }));

    REQUIRE(a.get() != b.get());
}

TEST(Node_GlobalCollectFromDeepNode, RuntimeFixture,
    "globalCollect должен искать от корня независимо от того, из какого узла он вызван.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branchA = nodes.factory.folder(fixture.root, "A");
    nodes.build(branchA).add<TestComponent>("a");

    const NodeId branchB = nodes.factory.folder(fixture.root, "B");
    nodes.build(branchB).add<TestComponent>("b");

    const NodeId deep = nodes.factory.folder(branchA, "Deep");
    nodes.build(deep).add<TestComponent>("deep");

    auto rootComponent = nodes.configure(fixture.root).require<TestComponent>("root");
    auto a = nodes.configure(branchA).require<TestComponent>("a");
    auto b = nodes.configure(branchB).require<TestComponent>("b");
    auto deepComponent = nodes.configure(deep).require<TestComponent>("deep");

    auto result = nodes.query.collect(nodes.ops.root(deep), typeKey<TestComponent>());

    REQUIRE(result.size() == 4);

    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == rootComponent.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == a.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == b.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == deepComponent.get(); }));
}

TEST(Node_GlobalCollectByRole, RuntimeFixture,
    "globalCollect должен находить все объекты, реализующие интерфейс, независимо от concrete-типа.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<TestImplA, TestAPI>(fixture.run_ctx.blueprints);
    BlueprintRegister::add<TestImplB, TestAPI>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestImplA>("a");
    nodes.build(fixture.root).add<TestImplB>("b");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestImplA>("c");

    auto a = nodes.configure(fixture.root).require<TestAPI>("a");
    auto b = nodes.configure(fixture.root).require<TestAPI>("b");
    auto c = nodes.configure(branch).require<TestAPI>("c");

    auto result = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<TestAPI>());

    REQUIRE(result.size() == 3);

    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == a.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestComponent>()) == b.get(); }));
    REQUIRE(std::ranges::any_of(result, [&](NodeId id) { return nodes.query.resolve(id, typeKey<TestAPI>()) == c.get(); }));
}

TEST(Node_GlobalCollectIgnoresInstanceName, RuntimeFixture, 
"Глобальный поиск должен находить все экземпляры компонента независимо от имени реализации.")
 {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("one");
    nodes.build(fixture.root).add<TestComponent>("two");
    nodes.build(fixture.root).add<TestComponent>("three");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("four");
    nodes.build(branch).add<TestComponent>("five");

    auto Node = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<TestComponent>());

    REQUIRE(Node.size() == 5);
}

// TEST(Node_RemoveDoesNotAffectParent, RuntimeFixture,
//     "Удаление компонента из дочерней ветки не должно удалять компонент родителя.") {

//     BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

//     nodes.build(fixture.root).add<TestComponent>("shared");

//     const NodeId branch = nodes.factory.folder(fixture.root, "branch");
//     nodes.build(branch).add<TestComponent>("shared");

//     nodes.ops.destroyBranch(nodes.query.find(branch, typeKey<TestComponent>(), "shared"));
//     REQUIRE(nodes.query.collect(branch, typeKey<TestComponent>()).empty());
//     REQUIRE(nodes.configure(fixture.root).find<TestComponent>("shared").exists());
// }

TEST(Node_RemoveShadowDoesNotRevealWrongComponent, RuntimeFixture, 
"После удаления локального компонента поиск должен корректно продолжить поиск у родителя. \
Удаление индекса дочернего компонента не должно повреждать или скрывать родительский объект.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("shared");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("shared");

    REQUIRE(nodes.configure(branch).find<TestComponent>("shared").exists());

    nodes.ops.destroyBranch(nodes.query.find(branch, typeKey<TestComponent>(), "shared"));

    REQUIRE(nodes.configure(branch).find<TestComponent>("shared").exists());
}

TEST(Node_ConfigureDeepTree, RuntimeFixture, 
"configureBranch должен вызвать configure для каждого компонента во всей ветке.\
Вызов должен корректно проходить через произвольную глубину дерева.") 
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    nodes.build(branch).add<TestComponent>("branch");

    const NodeId child = nodes.factory.folder(fixture.root, "Child");
    nodes.build(child).add<TestComponent>("child");

    nodes.ops.configureBranch(fixture.root);

    REQUIRE(nodes.configure(fixture.root).require<TestComponent>("root")->configured);
    REQUIRE(nodes.configure(branch).require<TestComponent>("branch")->configured);
    REQUIRE(nodes.configure(child).require<TestComponent>("child")->configured);
}

TEST(Node_ConfigureDoesNotConfigureTwice, RuntimeFixture, 
"Повторный вызов configureBranch не должен приводить к неконтролируемому состоянию компонента. \
Компонент должен сохранять корректное сконфигурированное состояние.")
{
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<TestComponent>(fixture.run_ctx.blueprints);

    nodes.build(fixture.root).add<TestComponent>();

    auto component = nodes.configure(fixture.root).require<TestComponent>();

    nodes.ops.configureBranch(fixture.root);

    REQUIRE(component->configured);

    nodes.ops.configureBranch(fixture.root);

    REQUIRE(component->configured);
}

}