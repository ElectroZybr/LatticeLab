#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {
struct FocusCamera { int value = 7; };
namespace Other { struct FocusCamera {}; }
struct FocusOwner {};
struct FocusConsumer {
    Focus<FocusCamera> camera;
    void configure(Node& node) { camera = node.focus<FocusCamera>(); }
};
template<class F> bool throwsFocus(F&& f) {
    try { f(); } catch (const Exception&) { return true; }
    return false;
}
}

TEST(Focus_RolesAndScopes, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    auto& root = fixture.root;
    const auto scope = root.getFocusScopeId();
    REQUIRE(scope != InvalidFocusScopeId);
    REQUIRE(root.makeFocusScope() == scope);
    REQUIRE(ctx.activeScope == scope);
    REQUIRE(ctx.activeChain == std::vector<FocusScopeId>{scope});
    auto& folder = root.addFolder("folder");
    REQUIRE(folder.getFocusScopeId() == InvalidFocusScopeId);
    auto a = folder.focus<FocusCamera>();
    auto b = folder.focus<FocusCamera>(typeKey<FocusCamera>());
    auto other = folder.focus<Other::FocusCamera>();
    const auto role = ctx.roles.find(typeKey<FocusCamera>());
    REQUIRE(role != ctx.roles.find(typeKey<Other::FocusCamera>()));
    REQUIRE(ctx.focusScopes.require(scope).roles.empty());
    fixture.blueprints.add<FocusCamera>();
    folder.add<FocusCamera>();
    REQUIRE(a.id() == InvalidObjectId); // Creation never assigns a Focus role.
    auto target = folder.find<FocusCamera>().node->getId();
    ctx.setFocus(scope, role, target);
    REQUIRE(a.get() == b.get());
    REQUIRE(a->value == 7);
    REQUIRE(!other);
    auto selected = folder.focus<FocusCamera>("selected");
    auto hovered = folder.focus<FocusCamera>("hovered");
    ctx.setFocus(scope, ctx.roles.find("selected"), target);
    REQUIRE(selected.id() == target);
    REQUIRE(!hovered);
    const auto action = folder.on("action", [] {});
    auto* node = ctx.objects.require(action).node;
    REQUIRE(node->makeFocusScope() != InvalidFocusScopeId);
    ctx.setFocus(scope, role, action);
    REQUIRE(a.id() == action);
    REQUIRE(!a); // An addressable binding is valid, but is not a Camera.
}

TEST(Focus_LocalAndGlobalMerge, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    auto& root = fixture.root;
    fixture.blueprints.add<FocusCamera>();
    root.add<FocusCamera>("one");
    root.add<FocusCamera>("two");
    auto one = root.find<FocusCamera>("one").node->getId();
    auto two = root.find<FocusCamera>("two").node->getId();
    auto& left = root.addFolder("left");
    auto& right = root.addFolder("right");
    auto ls = left.makeFocusScope(), rs = right.makeFocusScope();
    auto l = left.focus<FocusCamera>(), r = right.focus<FocusCamera>();
    auto global = ctx.focus<FocusCamera>();
    auto role = ctx.roles.find(typeKey<FocusCamera>());
    ctx.setFocus(root.getFocusScopeId(), role, one);
    ctx.setFocus(rs, role, two);
    REQUIRE(l.id() == one);
    REQUIRE(r.id() == two);
    ctx.activateFocus(ls);
    REQUIRE(global.id() == one);
    ctx.setFocus(ls, role, InvalidObjectId);
    REQUIRE(!l);
    REQUIRE(!global);
    ctx.resetFocus(ls, role);
    REQUIRE(l.id() == one);
    ctx.activateFocus(rs);
    REQUIRE(global.id() == two);
    auto secondary = ctx.getOrCreateRole("secondary");
    ctx.setFocus(rs, secondary, one);
    ctx.setFocus(ls, role, two);
    REQUIRE(global.id() == two);
    ctx.setFocus(rs, role, one);
    REQUIRE(global.id() == one);
    REQUIRE(ctx.resolvedRoles[secondary] == one);
    REQUIRE(r.id() == one);
    ctx.resetFocus(rs, role);
    ctx.setFocus(root.getFocusScopeId(), role, two);
    REQUIRE(global.id() == two);
    REQUIRE(ctx.resolvedRoles.size() == ctx.roles.size());
    REQUIRE(ctx.resolvedRoles[ctx.getOrCreateRole("new")] == InvalidObjectId);
}

TEST(Focus_TopologyAndConfigure, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    auto& root = fixture.root;
    fixture.blueprints.add<FocusCamera>();
    fixture.blueprints.add<FocusConsumer>();
    root.add<FocusCamera>("one");
    root.add<FocusCamera>("two");
    auto one = root.find<FocusCamera>("one").node->getId();
    auto two = root.find<FocusCamera>("two").node->getId();
    auto& middle = root.addFolder("middle");
    auto& leaf = middle.addFolder("leaf");
    auto leafScope = leaf.makeFocusScope();
    middle.add<FocusConsumer>();
    auto consumer = middle.find<FocusConsumer>();
    consumer->configure(*consumer.node);
    auto role = ctx.roles.find(typeKey<FocusCamera>());
    ctx.setFocus(root.getFocusScopeId(), role, one);
    ctx.activateFocus(leafScope);
    auto middleScope = middle.makeFocusScope();
    ctx.setFocus(middleScope, role, two);
    const std::vector<FocusScopeId> expected{root.getFocusScopeId(), middleScope, leafScope};
    REQUIRE(ctx.activeChain == expected);
    REQUIRE(ctx.focus<FocusCamera>().id() == two);
    REQUIRE(consumer->camera.id() == one);
    consumer->configure(*consumer.node);
    REQUIRE(consumer->camera.id() == two);
}

TEST(Focus_ValidationAndDeletion, RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    auto& root = fixture.root;
    fixture.blueprints.add<FocusCamera>();
    fixture.blueprints.add<FocusOwner>();
    root.add<FocusCamera>();
    const auto target = root.find<FocusCamera>().node->getId();
    root.add<FocusOwner>();
    auto* owner = root.find<FocusOwner>().node;
    owner->add<FocusOwner>();
    auto* child = owner->find<FocusOwner>().node;
    auto parentScope = owner->makeFocusScope();
    auto childScope = child->makeFocusScope();
    auto handle = ctx.focus<FocusCamera>();
    auto role = ctx.roles.find(typeKey<FocusCamera>());
    ctx.setFocus(parentScope, role, target);
    ctx.activateFocus(childScope);
    REQUIRE(throwsFocus([&] { ctx.setFocus(parentScope, role, 999999); }));
    REQUIRE(throwsFocus([&] { ctx.setFocus(InvalidFocusScopeId, role, target); }));
    REQUIRE(throwsFocus([&] { ctx.setFocus(parentScope, InvalidRoleId, target); }));
    REQUIRE(throwsFocus([&] { ctx.resetFocus(parentScope, InvalidRoleId); }));
    REQUIRE(throwsFocus([&] { ctx.activateFocus(InvalidFocusScopeId); }));
    REQUIRE(handle.id() == target);
    owner->remove<FocusOwner>();
    REQUIRE(ctx.activeScope == parentScope);
    REQUIRE(!ctx.focusScopes.get(childScope));
    root.remove<FocusCamera>();
    REQUIRE(!handle);
    REQUIRE(ctx.focusScopes.require(parentScope).roles.front().target == InvalidObjectId);
    root.add<FocusCamera>();
    REQUIRE(root.find<FocusCamera>().node->getId() == target);
    REQUIRE(!handle);
    ctx.setFocus(root.getFocusScopeId(), role, target);
    REQUIRE(!handle); // Deletion left Empty, not Unset.
    root.remove<FocusOwner>();
    REQUIRE(ctx.activeScope == root.getFocusScopeId());
    REQUIRE(handle.id() == target);
}

TEST(Focus_SlotReplacement, RuntimeFixture) {
    auto& root = fixture.root;
    auto& ctx = fixture.run_ctx;
    fixture.blueprints.add<FocusCamera>();
    auto slot = root.slot<FocusCamera>();
    auto handle = root.focus<FocusCamera>();
    auto role = ctx.roles.find(typeKey<FocusCamera>());
    ctx.setFocus(root.getFocusScopeId(), role, slot.node->getId());
    REQUIRE(!handle);
    slot.use(typeKey<FocusCamera>());
    REQUIRE(handle.get() == slot.get());
    slot->value = 99;
    slot.use(typeKey<FocusCamera>());
    REQUIRE(handle.get() == slot.get());
    REQUIRE(handle->value == 7);
}
}
