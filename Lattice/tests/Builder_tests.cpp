#include <stdexcept>

#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/Fixture.hpp>
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

}

TEST(Builder_BuildsAndConfiguresBranch, RuntimeFixture,
    "Builder должен построить и настроить одиночную ветку.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId blueprint = BlueprintRegister::add<BuilderTestComponent>(
        fixture.run_ctx.blueprints
    );

    const NodeId branch = nodes.builder.build(fixture.root, blueprint, "one");
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

    const NodeId root = nodes.builder.build(InvalidNodeId, blueprint);

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
    nodes.builder.build(fixture.root, blueprint, "same");

    bool thrown = false;
    try {
        nodes.builder.build(fixture.root, blueprint, "same");
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

}
