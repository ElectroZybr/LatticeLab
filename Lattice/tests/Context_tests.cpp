#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
struct ContextDummy {};

TEST(Context_ExplicitAssignments, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    auto& root = fixture.root;
    fixture.blueprints.add<ContextDummy>();
    root.add<ContextDummy>();
    REQUIRE(ctx.roles.size() == 0);
    int value = 1;
    const auto binding = root.bind("value", &value);
    const auto action = root.on("action", [] {});
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, ctx.roles.find("value")) == binding);
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, ctx.roles.find("action")) == action);
    auto& local = root.addFolder("local");
    auto scope = local.makeFocusScope();
    auto& child = local.addFolder("child");
    child.setFocus("value", binding);
    const auto role = ctx.roles.find("value");
    REQUIRE(ctx.resolveFocus(scope, role) == binding);
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, role) == binding);
    ctx.activateFocus(scope);
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, role) == binding);
    child.setFocus("value", InvalidObjectId);
    REQUIRE(ctx.resolveFocus(scope, role) == InvalidObjectId);
    child.setFocus("value", action);
    REQUIRE(ctx.roles.find("value") == role);
    REQUIRE(ctx.resolveFocus(scope, role) == action);
    bool rejected = false;
    try { child.setFocus("invalid", 999999); } catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(ctx.roles.find("invalid") == InvalidRoleId);
    ctx.printTree();
}
}

namespace Lattice {
TEST(Context_AutoBindingsUseNearestScope, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    int rootCalls = 0, calls = 0, value = 0, changed = 0;
    const auto rootAction = fixture.root.on("move", [&] { ++rootCalls; });
    auto& local = fixture.root.addFolder("local");
    const auto scope = local.makeFocusScope();
    auto& child = local.addFolder("consumer");
    const auto action = child.on("move", [&] { ++calls; });
    const auto role = ctx.roles.find("move");
    REQUIRE(ctx.resolveFocus(scope, role) == action);
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, role) == rootAction);
    ctx.activateFocus(scope);
    ctx.bindings.invoke(ctx.resolveFocus(InvalidFocusScopeId, role));
    REQUIRE(calls == 1);
    REQUIRE(rootCalls == 0);
    child.setFocus("move", InvalidObjectId);
    REQUIRE(child.on("move", [&] { calls += 2; }) == action);
    ctx.bindings.invoke(ctx.resolveFocus(InvalidFocusScopeId, role));
    REQUIRE(calls == 3);
    REQUIRE(ctx.focusScopes.require(scope).roles.size() == 1);
    const auto binding = child.bind("value", &value);
    const auto valueRole = ctx.roles.find("value");
    REQUIRE(ctx.resolveFocus(scope, valueRole) == binding);
    child.setFocus("value", InvalidObjectId);
    REQUIRE(child.bind("value", &value, [&](int v) { changed = v; }) == binding);
    REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, valueRole) == binding);
    ctx.bindings.set(binding, 7);
    REQUIRE(value == 7);
    REQUIRE(changed == 7);
    REQUIRE(ctx.focusScopes.require(scope).roles.size() == 2);
}
}
