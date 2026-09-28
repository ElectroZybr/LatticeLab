#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <Lattice/Lattice.hpp>

namespace Lattice {

TEST(Exports_ActionContextAndTypedArguments, RuntimeFixture,
    "Action должен получать типизированные аргументы и иметь возможность изменить контекст вызова.") {
    const NodeId child = fixture.run_ctx.nodes.factory.folder(fixture.root, "child");
    std::string received;

    const ExportId action = fixture.build(fixture.root).action<std::string>(
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
    REQUIRE(resolved.requiredArguments == 1);
    REQUIRE(resolved.argumentTypes.front().is<std::string>());

    ActionContext context{fixture.root};
    const std::vector<Value> arguments{Value{"child"}};
    resolved.invoke(resolved.object, context, arguments);

    REQUIRE(received == "child");
    REQUIRE(context.node() == child);
    REQUIRE(context.output().size() == 1);
    REQUIRE(std::get<Value>(context.output().front()).get<std::string>() == "done");
}

TEST(Exports_OptionalTrailingActionArgument, RuntimeFixture,
    "Optional аргумент действия должен принимать как значение, так и его отсутствие.") {
    std::optional<std::string> received;
    const ExportId action = fixture.build(fixture.root).action<std::optional<std::string>>(
        "run",
        [&](std::optional<std::string> name) { received = std::move(name); }
    );

    ResolvedExport resolved = fixture.run_ctx.nodes.exports.resolve(action);
    REQUIRE(resolved.argumentTypes.size() == 1);
    REQUIRE(resolved.requiredArguments == 0);
    REQUIRE(resolved.argumentTypes.front().is<std::string>());

    ActionContext context{fixture.root};
    resolved.invoke(resolved.object, context, {});
    REQUIRE(!received);

    const std::vector<Value> arguments{Value{"case"}};
    resolved.invoke(resolved.object, context, arguments);
    REQUIRE(received == "case");
}

TEST(Exports_ActionContextStructuredView, RuntimeFixture,
    "ActionContext должен возвращать non-owning типизированное представление.") {
    struct View {
        int value = 0;
    } view{42};

    const ExportId action = fixture.build(fixture.root).action(
        "view",
        [&](ActionContext& context) {
            context.present(view);
        }
    );

    ActionContext context{fixture.root};
    fixture.run_ctx.nodes.exports.invoke(action, context);

    REQUIRE(context.output().size() == 1);
    const auto& output = std::get<ActionView>(context.output().front());
    REQUIRE(output.is<View>());
    REQUIRE(output.as<View>().value == 42);
}

TEST(Exports_LegacyActionCallback, RuntimeFixture,
    "Action без аргументов должен продолжать принимать обычный callback void().") {
    bool invoked = false;
    const ExportId action = fixture.build(fixture.root).action(
        "legacy",
        [&] { invoked = true; }
    );

    fixture.run_ctx.nodes.exports.invoke(action);
    REQUIRE(invoked);
}

TEST(Exports_LocalScopeIsCreatedLazily, RuntimeFixture,
    "Первый локальный export должен создать scope владельца и быть виден только из его ветки.") {
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    bool invoked = false;

    REQUIRE(nodes.context.findScope(branch) == InvalidContextScopeId);
    fixture.build(branch).action("local", [&] { invoked = true; });

    const ContextScopeId scope = nodes.context.findScope(branch);
    REQUIRE(scope != InvalidContextScopeId);

    ExportsView exports = nodes.configure(branch).exports();
    const RoleId role = exports.role("local");
    REQUIRE(!exports.resolve(role, fixture.root));

    ResolvedExport local = exports.resolve(role, branch);
    REQUIRE(local.invoke != nullptr);
    ActionContext context{branch};
    local.invoke(local.object, context, {});
    REQUIRE(invoked);
}

TEST(Exports_GlobalDoesNotCreateOwnerScope, RuntimeFixture,
    "Глобальный export должен быть доступен из любой ветки без создания scope владельца.") {
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = nodes.factory.folder(fixture.root, "owner");
    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    bool invoked = false;

    fixture.build(owner).globalAction("global", [&] { invoked = true; });
    REQUIRE(nodes.context.findScope(owner) == InvalidContextScopeId);

    ExportsView exports = nodes.configure(branch).exports();
    const RoleId role = exports.role("global");
    ResolvedExport global = exports.resolve(role, branch);
    REQUIRE(global.invoke != nullptr);

    const auto visible = exports.available(branch);
    auto visibleIt = visible.begin();
    REQUIRE(visibleIt != visible.end());
    REQUIRE(exports.name(*visibleIt) == "global");
    REQUIRE(visibleIt->global);
    REQUIRE(visibleIt->state == ContextResolutionState::Resolved);
    ++visibleIt;
    REQUIRE(visibleIt == visible.end());

    ActionContext context{branch};
    global.invoke(global.object, context, {});
    REQUIRE(invoked);
}

TEST(Exports_LocalShadowsGlobal, RuntimeFixture,
    "Локальный export с тем же именем должен перекрывать глобальный внутри своей ветки.") {
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId globalOwner = nodes.factory.folder(fixture.root, "global-owner");
    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    int selected = 0;

    const ExportId globalId = fixture.build(globalOwner).globalAction("select", [&] { selected = 1; });
    const ExportId localId = fixture.build(branch).action("select", [&] { selected = 2; });

    ExportsView exports = nodes.configure(branch).exports();
    const RoleId role = exports.role("select");

    ActionContext rootContext{fixture.root};
    ResolvedExport global = exports.resolve(role, fixture.root);
    global.invoke(global.object, rootContext, {});
    REQUIRE(selected == 1);

    ActionContext branchContext{branch};
    ResolvedExport local = exports.resolve(role, branch);
    local.invoke(local.object, branchContext, {});
    REQUIRE(selected == 2);

    REQUIRE(exports.resolveExport(localId, branch));
    REQUIRE(!exports.resolveExport(globalId, branch));
    REQUIRE(exports.resolveExport(globalId, fixture.root));

    const auto visible = exports.available(branch);
    auto visibleIt = visible.begin();
    REQUIRE(visibleIt != visible.end());
    REQUIRE(visibleIt->owner == branch);
    REQUIRE(!visibleIt->global);
    ++visibleIt;
    REQUIRE(visibleIt == visible.end());
}

TEST(Exports_GlobalAliasSharesExport, RuntimeFixture,
    "Alias должен разрешаться в тот же export без дублирования action.") {
    auto& nodes = fixture.run_ctx.nodes;
    const NodeId owner = nodes.factory.folder(fixture.root, "owner");
    const NodeId branch = nodes.factory.folder(fixture.root, "branch");
    int invoked = 0;

    auto build = fixture.build(owner);
    const ExportId target = build.globalAction("quit", [&] { ++invoked; });
    build.globalAlias("exit", target);

    ExportsView exports = nodes.configure(branch).exports();
    ResolvedExport quit = exports.resolve(exports.role("quit"), branch);
    ResolvedExport exit = exports.resolve(exports.role("exit"), branch);

    REQUIRE(quit);
    REQUIRE(exit);
    REQUIRE(exports.resolveExport(target, branch));
    REQUIRE(exports.name(target) == "quit");

    ActionContext context{branch};
    quit.invoke(quit.object, context, {});
    exit.invoke(exit.object, context, {});
    REQUIRE(invoked == 2);
}

}
