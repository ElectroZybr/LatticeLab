#include <algorithm>

#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

struct DependencyTarget : Component {};

struct DependencyConsumer : Component {
    Ref<DependencyTarget> target;

    void configure(NodeConfigure node) {
        target = node.require<DependencyTarget>("target");
    }
};

struct IndependentComponent : Component {};

struct OwnedDependencyChild : Component {};

struct BranchDependencyConsumer : Component {
    explicit BranchDependencyConsumer(NodeBuild node) {
        node.add<OwnedDependencyChild>();
    }

    void configure(NodeConfigure node) {
        node.require<DependencyTarget>("target");
    }
};

struct TransitiveDependencyConsumer : Component {
    void configure(NodeConfigure node) {
        node.require<BranchDependencyConsumer>("consumer");
    }
};

}

TEST(DependencyGraph_StoresBothDirectionsAndDeduplicates, RuntimeFixture,
    "DependencyGraph должен хранить обе стороны одного уникального ребра.") {
    DependencyGraph graph;

    graph.add(3, 7);
    graph.add(3, 7);

    REQUIRE(graph.dependencies(3).size() == 1);
    REQUIRE(graph.dependencies(3).front() == 7);
    REQUIRE(graph.dependents(7).size() == 1);
    REQUIRE(graph.dependents(7).front() == 3);

    graph.add(11, 3);
    graph.clearDependencies(3);

    REQUIRE(graph.dependencies(3).empty());
    REQUIRE(graph.dependents(7).empty());
    REQUIRE(graph.dependencies(11).size() == 1);
    REQUIRE(graph.dependents(3).size() == 1);

    graph.removeNode(3);

    REQUIRE(graph.dependencies(3).empty());
    REQUIRE(graph.dependents(3).empty());
    REQUIRE(graph.dependencies(11).empty());
}

TEST(DependencyGraph_NodeConfigureRegistersResolvedReference, RuntimeFixture,
    "NodeConfigure::require должен зарегистрировать зависимость владельца Ref от цели.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId targetBlueprint = BlueprintRegister::add<DependencyTarget, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId consumerBlueprint = BlueprintRegister::add<DependencyConsumer, Component>(
        fixture.run_ctx.blueprints
    );

    const NodeId target = nodes.builder.add(fixture.root, targetBlueprint, "target");
    const NodeId consumer = nodes.builder.add(fixture.root, consumerBlueprint, "consumer");

    REQUIRE(nodes.dependencies.dependencies(consumer).size() == 1);
    REQUIRE(nodes.dependencies.dependencies(consumer).front() == target);
    REQUIRE(nodes.dependencies.dependents(target).size() == 1);
    REQUIRE(nodes.dependencies.dependents(target).front() == consumer);

    nodes.configure(consumer).require<DependencyTarget>("target");

    REQUIRE(nodes.dependencies.dependencies(consumer).size() == 1);
    REQUIRE(nodes.dependencies.dependents(target).size() == 1);
}

TEST(DependencyGraph_DestroyRemovesEdgesBeforeNodeIdReuse, RuntimeFixture,
    "Уничтожение узла должно очистить граф до повторного использования его NodeId.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId targetBlueprint = BlueprintRegister::add<DependencyTarget, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId consumerBlueprint = BlueprintRegister::add<DependencyConsumer, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId independentBlueprint = BlueprintRegister::add<IndependentComponent, Component>(
        fixture.run_ctx.blueprints
    );

    const NodeId target = nodes.builder.add(fixture.root, targetBlueprint, "target");
    const NodeId consumer = nodes.builder.add(fixture.root, consumerBlueprint, "consumer");

    nodes.ops.destroyBranch(consumer);

    REQUIRE(nodes.dependencies.dependencies(consumer).empty());
    REQUIRE(nodes.dependencies.dependents(consumer).empty());
    REQUIRE(nodes.dependencies.dependents(target).empty());

    const NodeId replacement = nodes.builder.add(fixture.root, independentBlueprint, "replacement");

    REQUIRE(replacement == consumer);
    REQUIRE(nodes.dependencies.dependencies(replacement).empty());
    REQUIRE(nodes.dependencies.dependents(replacement).empty());
}

TEST(DependencyGraph_RemovalClosureIncludesOwnedAndTransitiveDependents, RuntimeFixture,
    "Removal closure должен рекурсивно объединять ownership-поддеревья и dependents.") {
    auto& nodes = fixture.run_ctx.nodes;
    const BlueprintId targetBlueprint = BlueprintRegister::add<DependencyTarget, Component>(
        fixture.run_ctx.blueprints
    );
    BlueprintRegister::add<OwnedDependencyChild, Component>(fixture.run_ctx.blueprints);
    const BlueprintId consumerBlueprint = BlueprintRegister::add<BranchDependencyConsumer, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId transitiveBlueprint = BlueprintRegister::add<TransitiveDependencyConsumer, Component>(
        fixture.run_ctx.blueprints
    );
    const BlueprintId independentBlueprint = BlueprintRegister::add<IndependentComponent, Component>(
        fixture.run_ctx.blueprints
    );

    const NodeId target = nodes.builder.add(fixture.root, targetBlueprint, "target");
    const NodeId consumer = nodes.builder.add(fixture.root, consumerBlueprint, "consumer");
    const NodeId ownedChild = nodes.registry.children(consumer).front();
    const NodeId transitive = nodes.builder.add(fixture.root, transitiveBlueprint, "transitive");
    const NodeId independent = nodes.builder.add(fixture.root, independentBlueprint, "independent");
    nodes.dependencies.add(target, transitive); // замыкает цикл target -> transitive -> consumer -> target

    const auto closure = nodes.ops.collectRemovalClosure(target);
    const auto contains = [&closure](NodeId node) {
        return std::ranges::find(closure, node) != closure.end();
    };

    REQUIRE(contains(target));
    REQUIRE(contains(consumer));
    REQUIRE(contains(ownedChild));
    REQUIRE(contains(transitive));
    REQUIRE(!contains(independent));
    REQUIRE(closure.size() == 4);
}

}
