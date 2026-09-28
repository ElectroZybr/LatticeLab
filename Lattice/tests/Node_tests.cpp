#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>


namespace Lattice {

class BasicTestComponent {
public:
    bool configured = false;

    void configure(NodeConfigure) {
        configured = true;
    }
};

TEST(Node_Add, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    REQUIRE(!nodes.configure(fixture.root).find<BasicTestComponent>().exists());

    fixture.build(fixture.root).add<BasicTestComponent>();

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>().exists());
    REQUIRE(nodes.registry.require(nodes.query.require(fixture.root, typeKey<BasicTestComponent>())).name.empty());
    REQUIRE(nodes.configure(fixture.root).require<BasicTestComponent>().exists());
}

TEST(Node_AddDuplicate, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();
    fixture.build(fixture.root).add<BasicTestComponent>();

    auto settings = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<BasicTestComponent>());
    REQUIRE(settings.size() == 1);
}

TEST(Node_CustomInstance, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("custom");

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>("custom").exists());
    REQUIRE(!nodes.configure(fixture.root).find<BasicTestComponent>("default").exists());
}

TEST(Node_InstanceIsolation, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("first");
    fixture.build(fixture.root).add<BasicTestComponent>("second");

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>("first").exists());
    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>("second").exists());
    REQUIRE(!nodes.configure(fixture.root).find<BasicTestComponent>("default").exists());

    auto settings = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<BasicTestComponent>());

    REQUIRE(settings.size() == 2);
}

TEST(Node_RequireMissing, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    bool thrown = false;

    try {
        nodes.configure(fixture.root).require<BasicTestComponent>();
    } catch (const ExceptionBase&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Node_RegisterAndAdd, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>().exists());
    REQUIRE(nodes.configure(fixture.root).require<BasicTestComponent>().exists());
}

TEST(Node_Configure, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);
    fixture.build(fixture.root).add<BasicTestComponent>();

    auto component = nodes.configure(fixture.root).require<BasicTestComponent>();

    REQUIRE(!component->configured);

    nodes.ops.configureBranch(fixture.root);

    REQUIRE(component->configured);
}

TEST(Node_GlobalCollect, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("first");
    fixture.build(fixture.root).add<BasicTestComponent>("second");

    auto Node = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<BasicTestComponent>());

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollect, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("root");

    const NodeId branch = fixture.root;

    fixture.build(branch).add<BasicTestComponent>("another");

    auto Node = nodes.query.collect(branch, typeKey<BasicTestComponent>());

    REQUIRE(Node.size() == 2);
}

TEST(Node_ChildVisibility, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    auto child = nodes.configure(fixture.root).find<BasicTestComponent>();

    REQUIRE(child.exists());
}

TEST(Node_ParentLookup, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");

    REQUIRE(nodes.configure(branch).find<BasicTestComponent>().exists());
    REQUIRE(nodes.configure(branch).require<BasicTestComponent>().exists());
}

TEST(Node_Shadowing, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    fixture.build(branch).add<BasicTestComponent>();

    auto parent = nodes.configure(fixture.root).find<BasicTestComponent>();
    auto child = nodes.configure(branch).find<BasicTestComponent>();

    REQUIRE(parent.exists());
    REQUIRE(child.exists());
}

TEST(Node_ChildInstanceLookup, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    fixture.build(branch).add<BasicTestComponent>("child");

    REQUIRE(nodes.configure(branch).find<BasicTestComponent>("child").exists());
    REQUIRE(nodes.configure(branch).find<BasicTestComponent>("root").exists());
    REQUIRE(!nodes.configure(branch).find<BasicTestComponent>("missing").exists());
}

TEST(Node_GlobalCollectNested, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    fixture.build(branch).add<BasicTestComponent>("child");

    auto Node = nodes.query.collect(nodes.ops.root(branch), typeKey<BasicTestComponent>());

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollectNested, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("root");

    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    fixture.build(branch).add<BasicTestComponent>("child");

    auto Node = nodes.query.collect(branch, typeKey<BasicTestComponent>());

    REQUIRE(Node.size() == 1);
}

TEST(Node_Remove, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>().exists());

    nodes.ops.destroyBranch(nodes.query.find(fixture.root, typeKey<BasicTestComponent>()));

    REQUIRE(!nodes.configure(fixture.root).find<BasicTestComponent>().exists());
}

TEST(Node_RemoveInstance, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>("first");
    fixture.build(fixture.root).add<BasicTestComponent>("second");

    nodes.ops.destroyBranch(nodes.query.find(fixture.root, typeKey<BasicTestComponent>(), "first"));

    REQUIRE(!nodes.configure(fixture.root).find<BasicTestComponent>("first").exists());
    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>("second").exists());

    auto settings = nodes.query.collect(nodes.ops.root(fixture.root), typeKey<BasicTestComponent>());

    REQUIRE(settings.size() == 1);
}

TEST(Node_RemoveMissing, RuntimeFixture) {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BasicTestComponent>(fixture.run_ctx.blueprints);

    fixture.build(fixture.root).add<BasicTestComponent>();

    const auto missing = nodes.query.find(fixture.root, typeKey<BasicTestComponent>(), "missing");
    REQUIRE(missing == InvalidNodeId);
    if (missing != InvalidNodeId) nodes.ops.destroyBranch(missing);

    REQUIRE(nodes.configure(fixture.root).find<BasicTestComponent>().exists());
}

}
