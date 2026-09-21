#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>


namespace Lattice {

class BasicTestComponent {
public:
    bool configured = false;

    void configure(Node&) {
        configured = true;
    }
};

TEST(Node_Add, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    REQUIRE(!fixture.root.find<BasicTestComponent>().exists());

    fixture.root.add<BasicTestComponent>();

    REQUIRE(fixture.root.find<BasicTestComponent>().exists());
    REQUIRE(fixture.root.find<BasicTestComponent>().node()->name().empty());
    REQUIRE(fixture.root.require<BasicTestComponent>().exists());
}

TEST(Node_AddDuplicate, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();
    fixture.root.add<BasicTestComponent>();

    auto settings = fixture.root.globalCollect<BasicTestComponent>();
    REQUIRE(settings.size() == 1);
}

TEST(Node_CustomInstance, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("custom");

    REQUIRE(fixture.root.find<BasicTestComponent>("custom").exists());
    REQUIRE(!fixture.root.find<BasicTestComponent>("default").exists());
}

TEST(Node_InstanceIsolation, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("first");
    fixture.root.add<BasicTestComponent>("second");

    REQUIRE(fixture.root.find<BasicTestComponent>("first").exists());
    REQUIRE(fixture.root.find<BasicTestComponent>("second").exists());
    REQUIRE(!fixture.root.find<BasicTestComponent>("default").exists());

    auto settings = fixture.root.globalCollect<BasicTestComponent>();

    REQUIRE(settings.size() == 2);
}

TEST(Node_RequireMissing, RuntimeFixture) {
    bool thrown = false;

    try {
        fixture.root.require<BasicTestComponent>();
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Node_RegisterAndAdd, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    REQUIRE(fixture.root.find<BasicTestComponent>().exists());
    REQUIRE(fixture.root.require<BasicTestComponent>().exists());
}

TEST(Node_Configure, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();
    fixture.root.add<BasicTestComponent>();

    auto component = fixture.root.require<BasicTestComponent>();

    REQUIRE(!component->configured);

    fixture.root.configureBranch();

    REQUIRE(component->configured);
}

TEST(Node_GlobalCollect, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("first");
    fixture.root.add<BasicTestComponent>("second");

    auto Node = fixture.root.globalCollect<BasicTestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollect, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("root");

    Node& branch = fixture.root;

    branch.add<BasicTestComponent>("another");

    auto Node = branch.folderCollect<BasicTestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_ChildVisibility, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    auto child = fixture.root.find<BasicTestComponent>();

    REQUIRE(child.exists());
}

TEST(Node_ParentLookup, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    Node& branch = fixture.root.addFolder("branch");

    REQUIRE(branch.find<BasicTestComponent>().exists());
    REQUIRE(branch.require<BasicTestComponent>().exists());
}

TEST(Node_Shadowing, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    Node& branch = fixture.root.addFolder("branch");
    branch.add<BasicTestComponent>();

    auto parent = fixture.root.find<BasicTestComponent>();
    auto child = branch.find<BasicTestComponent>();

    REQUIRE(parent.exists());
    REQUIRE(child.exists());
}

TEST(Node_ChildInstanceLookup, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<BasicTestComponent>("child");

    REQUIRE(branch.find<BasicTestComponent>("child").exists());
    REQUIRE(branch.find<BasicTestComponent>("root").exists());
    REQUIRE(!branch.find<BasicTestComponent>("missing").exists());
}

TEST(Node_GlobalCollectNested, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<BasicTestComponent>("child");

    auto Node = branch.globalCollect<BasicTestComponent>();

    REQUIRE(Node.size() == 2);
}

TEST(Node_folderCollectNested, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("root");

    Node& branch = fixture.root.addFolder("branch");
    branch.add<BasicTestComponent>("child");

    auto Node = branch.folderCollect<BasicTestComponent>();

    REQUIRE(Node.size() == 1);
}

TEST(Node_Remove, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    REQUIRE(fixture.root.find<BasicTestComponent>().exists());

    fixture.root.remove<BasicTestComponent>();

    REQUIRE(!fixture.root.find<BasicTestComponent>().exists());
}

TEST(Node_RemoveInstance, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>("first");
    fixture.root.add<BasicTestComponent>("second");

    fixture.root.remove<BasicTestComponent>("first");

    REQUIRE(!fixture.root.find<BasicTestComponent>("first").exists());
    REQUIRE(fixture.root.find<BasicTestComponent>("second").exists());

    auto settings = fixture.root.globalCollect<BasicTestComponent>();

    REQUIRE(settings.size() == 1);
}

TEST(Node_RemoveMissing, RuntimeFixture) {
    fixture.blueprints.add<BasicTestComponent>();

    fixture.root.add<BasicTestComponent>();

    fixture.root.remove<BasicTestComponent>("missing");

    REQUIRE(fixture.root.find<BasicTestComponent>().exists());
}

}