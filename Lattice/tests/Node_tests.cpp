#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>


namespace Lattice {

class TestComponent {
public:
    bool configured = false;

    void configure(Node&) {
        configured = true;
    }
};

TEST(Node_Add, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    REQUIRE(!fixture.root.find<TestComponent>().exists());

    fixture.root.add<TestComponent>();

    REQUIRE(fixture.root.find<TestComponent>().exists());
    REQUIRE(fixture.root.require<TestComponent>().exists());
}

TEST(Node_AddDuplicate, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();
    fixture.root.add<TestComponent>();

    auto settings = fixture.root.globalCollect<TestComponent>();
    REQUIRE(settings.size() == 1);
}

TEST(Node_CustomInstance, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("custom");

    REQUIRE(fixture.root.find<TestComponent>("custom").exists());
    REQUIRE(!fixture.root.find<TestComponent>("default").exists());
}

TEST(Node_InstanceIsolation, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("first");
    fixture.root.add<TestComponent>("second");

    REQUIRE(fixture.root.find<TestComponent>("first").exists());
    REQUIRE(fixture.root.find<TestComponent>("second").exists());
    REQUIRE(!fixture.root.find<TestComponent>("default").exists());

    auto settings = fixture.root.globalCollect<TestComponent>();

    REQUIRE(settings.size() == 2);
}

TEST(Node_RequireMissing, RuntimeFixture) {
    bool thrown = false;

    try {
        fixture.root.require<TestComponent>();
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Node_RegisterAndAdd, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    REQUIRE(fixture.root.find<TestComponent>().exists());
    REQUIRE(fixture.root.require<TestComponent>().exists());
}

TEST(Node_Configure, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();
    fixture.root.add<TestComponent>();

    auto component = fixture.root.require<TestComponent>();

    REQUIRE(!component->configured);

    fixture.root.configureAll();

    REQUIRE(component->configured);
}

TEST(Node_GlobalCollect, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("first");
    fixture.root.add<TestComponent>("second");

    auto Node = fixture.root.globalCollect<TestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollect, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("root");

    Node& branch = fixture.root;

    branch.add<TestComponent>("another");

    auto Node = branch.folderCollect<TestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_ChildVisibility, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    auto child = fixture.root.find<TestComponent>();

    REQUIRE(child.exists());
}

TEST(Node_ParentLookup, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    Node& branch = fixture.root.addFolder("branch");

    REQUIRE(branch.find<TestComponent>().exists());
    REQUIRE(branch.require<TestComponent>().exists());
}

TEST(Node_Shadowing, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    Node& branch = fixture.root.addFolder("branch");
    branch.add<TestComponent>();

    auto parent = fixture.root.find<TestComponent>();
    auto child = branch.find<TestComponent>();

    REQUIRE(parent.exists());
    REQUIRE(child.exists());
}

TEST(Node_ChildInstanceLookup, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<TestComponent>("child");

    REQUIRE(branch.find<TestComponent>("child").exists());
    REQUIRE(branch.find<TestComponent>("root").exists());
    REQUIRE(!branch.find<TestComponent>("missing").exists());
}

TEST(Node_GlobalCollectNested, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<TestComponent>("child");

    auto Node = branch.globalCollect<TestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollectNested, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<TestComponent>("child");

    auto Node = branch.folderCollect<TestComponent>();

    REQUIRE(Node.size() == 1);
}

TEST(Node_Remove, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    REQUIRE(fixture.root.find<TestComponent>().exists());

    fixture.root.remove<TestComponent>();

    REQUIRE(!fixture.root.find<TestComponent>().exists());
}

TEST(Node_RemoveInstance, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>("first");
    fixture.root.add<TestComponent>("second");

    fixture.root.remove<TestComponent>("first");

    REQUIRE(!fixture.root.find<TestComponent>("first").exists());
    REQUIRE(fixture.root.find<TestComponent>("second").exists());

    auto settings = fixture.root.globalCollect<TestComponent>();

    REQUIRE(settings.size() == 1);
}

TEST(Node_RemoveMissing, RuntimeFixture) {
    fixture.blueprints.blueprint<TestComponent>();

    fixture.root.add<TestComponent>();

    fixture.root.remove<TestComponent>("missing");

    REQUIRE(fixture.root.find<TestComponent>().exists());
}

}