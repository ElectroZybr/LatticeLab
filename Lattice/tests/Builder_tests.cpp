#include <stdexcept>

#include <Lattice/Lattice.hpp>
#include <Lattice/tests/RuntimeFixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

struct BuilderTestComponent {
    bool configured = false;

    void configure(NodeConfigure) {
        configured = true;
    }
};

struct BatchTestComponent {
    inline static int constructed = 0;
    inline static int expectedAtConfigure = 0;
    inline static bool configuredAfterFullBuild = true;

    BatchTestComponent() {
        ++constructed;
    }

    void configure(NodeConfigure) {
        configuredAfterFullBuild &= constructed == expectedAtConfigure;
    }
};

struct FailingConfigureComponent {
    void configure(NodeConfigure) {
        throw std::runtime_error("configure failed");
    }
};

struct ChildrenAddComponent : Component {
    bool configured = false;

    void configure(NodeConfigure) {
        configured = true;
    }
};

struct OtherChildComponent : Component {};

struct ChildrenOwnerComponent : Component {
    Children<ChildrenAddComponent> children;
    int configureCalls = 0;

    void configure(NodeConfigure node) {
        children = node.children<ChildrenAddComponent>();
        ++configureCalls;
    }
};

struct FailingCreateOnceComponent : Component {
    inline static bool fail = true;

    explicit FailingCreateOnceComponent(NodeBuild node) {
        node.add<BuilderTestComponent>("partial-child");

        if (fail) {
            fail = false;
            throw std::runtime_error("create failed");
        }
    }
};

}

TEST(Builder_BuildsAndConfiguresBranch, RuntimeFixture,
    "Builder должен построить и настроить одиночную ветку.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints
    );

    const NodeId branch = nodes.builder.add(fixture.root, blueprint, "one");
    auto component = nodes.configure(fixture.root).require<BuilderTestComponent>("one");

    REQUIRE(branch != InvalidNodeId);
    REQUIRE(component->configured);
}

TEST(Builder_BuildsRoot, RuntimeFixture,
    "Builder должен уметь построить корневой компонент без особого пути.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints
    );

    const NodeId root = nodes.builder.add(InvalidNodeId, blueprint);

    REQUIRE(nodes.registry.require(root).parent == InvalidNodeId);
    REQUIRE(nodes.configure(root).resolve<BuilderTestComponent>(root)->configured);

    nodes.ops.destroyBranch(root);
}

TEST(Builder_BatchBuildsBeforeConfigure, RuntimeFixture,
    "Batch должен построить все ветки до начала configure.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BatchTestComponent>(
        fixture.run_ctx.blueprints
    );
    BatchTestComponent::constructed = 0;
    BatchTestComponent::expectedAtConfigure = 2;
    BatchTestComponent::configuredAfterFullBuild = true;

    auto batch = nodes.builder.begin();
    batch.add(fixture.root, blueprint, "one");
    batch.add(fixture.root, blueprint, "two");
    batch.commit();

    REQUIRE(BatchTestComponent::constructed == 2);
    REQUIRE(BatchTestComponent::configuredAfterFullBuild);
}

TEST(Builder_ConfigureFailureRollsBackBatch, RuntimeFixture,
    "Ошибка configure должна удалить все ветки текущего batch.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId regular = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints,
        "BuilderRollbackRegular"
    );
    const BlueprintId failing = BlueprintRegister::add<FailingConfigureComponent>(
        fixture.run_ctx.blueprints
    );

    bool thrown = false;
    try {
        auto batch = nodes.builder.begin();
        batch.add(fixture.root, regular, "regular");
        batch.add(fixture.root, failing, "failing");
        batch.commit();
    } catch (const std::runtime_error&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(nodes.registry.find("regular", fixture.root, regular) == InvalidNodeId);
    REQUIRE(nodes.registry.find("failing", fixture.root, failing) == InvalidNodeId);
}

TEST(Builder_RejectsExistingBranch, RuntimeFixture,
    "Builder не должен принимать существующую ветку за созданную транзакцией.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints,
        "BuilderDuplicate"
    );
    nodes.builder.add(fixture.root, blueprint, "same");

    bool thrown = false;
    try {
        nodes.builder.add(fixture.root, blueprint, "same");
    } catch (const ExceptionBase&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(nodes.registry.find("same", fixture.root, blueprint) != InvalidNodeId);
}

TEST(Builder_UncommittedBatchRollsBack, RuntimeFixture,
    "Незавершённый batch должен откатываться при выходе из scope.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints,
        "BuilderUncommitted"
    );

    {
        auto batch = nodes.builder.begin();
        batch.add(fixture.root, blueprint, "temporary");
    }

    REQUIRE(nodes.registry.find("temporary", fixture.root, blueprint) == InvalidNodeId);
}

TEST(Children_AddBuildsConfiguredChildOfFixedType, RuntimeFixture,
    "Children<T>::add должен создавать настроенного непосредственного ребёнка типа T.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<ChildrenAddComponent, Component>(
        fixture.run_ctx.blueprints
    );
    auto children = nodes.configure(fixture.root).children<ChildrenAddComponent>();

    const NodeId child = children.add("one");

    REQUIRE(child != InvalidNodeId);
    REQUIRE(nodes.registry.require(child).parent == fixture.root);
    REQUIRE(nodes.registry.require(child).bp == blueprint);
    REQUIRE(children.size() == 1);
    REQUIRE(children[0]->configured);

    const NodeId unnamed = children.add();

    REQUIRE(unnamed != InvalidNodeId);
    REQUIRE(nodes.registry.require(unnamed).name.empty());
    REQUIRE(children.size() == 2);
    REQUIRE(children[1]->configured);
}

TEST(Children_DelRetiresDirectChildAndUpdatesSnapshot, RuntimeFixture,
    "Children<T>::del должен удалить непосредственного ребёнка и обновить handler.") {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<ChildrenAddComponent, Component>(fixture.run_ctx.blueprints);
    auto children = nodes.configure(fixture.root).children<ChildrenAddComponent>();

    const NodeId first = children.add("first");
    const NodeId second = children.add("second");
    auto* secondObject = children[1];

    children.del(first);

    REQUIRE(nodes.registry.get(first) != nullptr);
    REQUIRE(nodes.registry.require(first).state == NodeState::Retiring);
    REQUIRE(nodes.registry.get(second) != nullptr);
    REQUIRE(children.size() == 1);
    REQUIRE(children[0] == secondObject);

    nodes.ops.maintain();
    REQUIRE(nodes.registry.get(first) == nullptr);
}

TEST(Children_DelRejectsWrongType, RuntimeFixture,
    "Children<T>::del не должен удалять ребёнка другого типа.") {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<ChildrenAddComponent, Component>(fixture.run_ctx.blueprints);
    const BlueprintId otherBlueprint = BlueprintRegister::add<OtherChildComponent, Component>(
        fixture.run_ctx.blueprints
    );
    const NodeId other = nodes.builder.add(fixture.root, otherBlueprint, "other");
    auto children = nodes.configure(fixture.root).children<ChildrenAddComponent>();

    bool thrown = false;
    try {
        children.del(other);
    } catch (const ExceptionBase&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(nodes.registry.get(other) != nullptr);
    REQUIRE(nodes.registry.require(other).state == NodeState::Active);
}

TEST(Children_RefreshesInConfigureAfterLocalGraphChanges, RuntimeFixture,
    "Children<T> должен обновляться в configure после изменения локального графа.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId childBlueprint = BlueprintRegister::add<ChildrenAddComponent, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId ownerBlueprint = BlueprintRegister::add<ChildrenOwnerComponent, Component>(
        fixture.run_ctx.blueprints
    );
    const NodeId owner = nodes.builder.add(fixture.root, ownerBlueprint, "owner");
    auto* component = static_cast<ChildrenOwnerComponent*>(
        nodes.query.resolve(owner, ownerBlueprint)
    );

    REQUIRE(component != nullptr);
    REQUIRE(component->configureCalls == 1);
    REQUIRE(component->children.empty());

    const NodeId child = nodes.builder.add(owner, childBlueprint, "external");
    REQUIRE(component->children.empty());

    nodes.ops.maintain();
    REQUIRE(component->configureCalls == 2);
    REQUIRE(component->children.size() == 1);

    nodes.builder.del(owner, child);
    REQUIRE(component->children.size() == 1);
    REQUIRE(nodes.registry.require(child).state == NodeState::Retiring);

    nodes.ops.maintain();
    REQUIRE(component->configureCalls == 3);
    REQUIRE(component->children.empty());
    REQUIRE(nodes.registry.get(child) == nullptr);
}

TEST(Builder_DelRejectsNonDirectChild, RuntimeFixture,
    "Builder::del должен принимать только непосредственного ребёнка указанного owner.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId parentBlueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints,
        "BuilderDelParent"
    );
    const BlueprintId childBlueprint = BlueprintRegister::add<ChildrenAddComponent, Component>(
        fixture.run_ctx.blueprints
    );
    const NodeId parent = nodes.builder.add(fixture.root, parentBlueprint, "parent");
    const NodeId child = nodes.builder.add(parent, childBlueprint, "child");

    bool thrown = false;
    try {
        nodes.builder.del(fixture.root, child);
    } catch (const ExceptionBase&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(nodes.registry.get(child) != nullptr);
    REQUIRE(nodes.registry.require(child).state == NodeState::Active);
}

TEST(Builder_CreateFailureLeavesNoRegistryGarbageAndCanRetry, RuntimeFixture,
    "Ошибка create должна полностью откатить ветку и освободить её имя для повторного add.") {
    auto& nodes = fixture.run_ctx.nodes;
    BlueprintRegister::add<BuilderTestComponent>(fixture.run_ctx.blueprints);
    const BlueprintId blueprint = BlueprintRegister::add<FailingCreateOnceComponent, Component>(
        fixture.run_ctx.blueprints
    );
    FailingCreateOnceComponent::fail = true;
    const size_t childrenBefore = nodes.registry.children(fixture.root).size();

    bool thrown = false;
    try {
        nodes.builder.add(fixture.root, blueprint, "retry");
    } catch (const std::runtime_error&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(nodes.registry.children(fixture.root).size() == childrenBefore);
    REQUIRE(nodes.registry.find("retry", fixture.root, blueprint) == InvalidNodeId);

    const NodeId child = nodes.builder.add(fixture.root, blueprint, "retry");

    REQUIRE(child != InvalidNodeId);
    REQUIRE(nodes.registry.find("retry", fixture.root, blueprint) == child);
    REQUIRE(nodes.registry.children(fixture.root).size() == childrenBefore + 1);
}

}
