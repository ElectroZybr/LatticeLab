#include <Lattice/tests/RuntimeFixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <CLI/include/Command.hpp>
#include <CLI/include/ValueParser.hpp>

namespace CLIPlugin {
namespace {

class PathComponent : public Lattice::Component {};

}

TEST(CLI_CommandLineArguments, Lattice::RuntimeFixture,
    "CLI parser должен снимать кавычки и экранирование до вызова C++ action.") {
    const auto parsed = parseArguments("one \"two words\" three\\ four");

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->size() == 3);
    REQUIRE((*parsed)[0] == "one");
    REQUIRE((*parsed)[1] == "two words");
    REQUIRE((*parsed)[2] == "three four");
    REQUIRE(!parseArguments("\"unfinished").has_value());
}

TEST(CLI_TreePathSyntax, Lattice::RuntimeFixture,
    "Строковый синтаксис путей должен оставаться в CLI поверх публичного TreeView API.") {
    auto& nodes = fixture.run_ctx.nodes;
    Lattice::BlueprintRegister::add<PathComponent>(fixture.run_ctx.blueprints);

    const Lattice::NodeId folder = nodes.factory.folder(fixture.root, "folder");
    const Lattice::NodeId componentId = nodes.factory.component(
        folder,
        nodes.blueprints.id<PathComponent>(),
        "one"
    );
    nodes.factory.resolve(componentId, nodes.blueprints.id<PathComponent>());
    const Lattice::TreeView tree = nodes.configure(fixture.root).tree();

    REQUIRE(resolveTreePath(tree, fixture.root, "folder") == folder);
    REQUIRE(resolveTreePath(tree, folder, "PathComponent:one") == componentId);
    REQUIRE(resolveTreePath(tree, componentId, "..") == folder);
    REQUIRE(resolveTreePath(tree, componentId, "/") == fixture.root);
    REQUIRE(resolveTreePath(tree, componentId, "Root/folder") == folder);
    REQUIRE(resolveTreePath(tree, fixture.root, "missing") == Lattice::InvalidNodeId);
}

}
