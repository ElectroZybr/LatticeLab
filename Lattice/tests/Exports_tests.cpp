#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>

namespace Lattice {

TEST(Exports_ActionContextAndTypedArguments, RuntimeFixture,
    "Action должен получать типизированные аргументы и иметь возможность изменить контекст вызова.") {
    const NodeId child = fixture.run_ctx.nodes.factory.folder(fixture.root, "child");
    std::string received;

    const ExportId action = fixture.run_ctx.nodes.build(fixture.root).action<std::string>(
        "navigate",
        [&](ActionContext& context, std::string path) {
            received = std::move(path);
            context.setNode(child);
            context.emit(Value{"done"});
        }
    );

    ResolvedExport resolved = fixture.run_ctx.nodes.exports.resolve(action);
    REQUIRE(resolved.invoke != nullptr);
    REQUIRE(resolved.argumentTypes.size() == 1);
    REQUIRE(resolved.argumentTypes.front().is<std::string>());

    ActionContext context{fixture.root};
    const std::vector<Value> arguments{Value{"child"}};
    resolved.invoke(resolved.object, context, arguments);

    REQUIRE(received == "child");
    REQUIRE(context.node() == child);
    REQUIRE(context.output().size() == 1);
    REQUIRE(context.output().front().get<std::string>() == "done");
}

TEST(Exports_LegacyActionCallback, RuntimeFixture,
    "Action без аргументов должен продолжать принимать обычный callback void().") {
    bool invoked = false;
    const ExportId action = fixture.run_ctx.nodes.build(fixture.root).action(
        "legacy",
        [&] { invoked = true; }
    );

    fixture.run_ctx.nodes.exports.invoke(action);
    REQUIRE(invoked);
}

}
