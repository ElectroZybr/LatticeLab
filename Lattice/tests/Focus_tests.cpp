// #include <Lattice/Tools/Fixture.hpp>
// #include <Lattice/Tools/Tests.hpp>

// namespace Lattice {
// namespace {
// struct FocusCamera { int value = 7; };
// namespace Other { struct FocusCamera {}; }
// struct FocusOwner {};
// struct FocusConsumer {
//     Focus<FocusCamera> camera;
//     void configure(Node& node) { camera = node.focus<FocusCamera>(); }
// };
// template<class F> bool throwsFocus(F&& f) {
//     try { f(); } catch (const ExceptionBase&) { return true; }
//     return false;
// }
// }

// TEST(Focus_RolesAndScopes, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     const auto scope = root.getFocusScopeId();
//     REQUIRE(scope != InvalidFocusScopeId);
//     REQUIRE(root.makeFocusScope() == scope);
//     REQUIRE(ctx.rootScope == scope);
//     REQUIRE(ctx.activeScopes.empty());
//     auto& folder = root.addFolder("folder");
//     REQUIRE(folder.getFocusScopeId() == InvalidFocusScopeId);
//     auto a = folder.focus<FocusCamera>();
//     auto b = folder.focus<FocusCamera>(typeKey<FocusCamera>());
//     auto other = folder.focus<Other::FocusCamera>();
//     const auto role = ctx.roles.find(typeKey<FocusCamera>());
//     REQUIRE(role != ctx.roles.find(typeKey<Other::FocusCamera>()));
//     REQUIRE(ctx.focusScopes.require(scope).roles.empty());
//     fixture.blueprints.add<FocusCamera>();
//     folder.add<FocusCamera>();
//     REQUIRE(a.id() == InvalidObjectId); // Creation never assigns a Focus role.
//     auto target = folder.find<FocusCamera>().node()->getId();
//     ctx.setFocus(scope, role, target);
//     REQUIRE(a.get() == b.get());
//     REQUIRE(a->value == 7);
//     REQUIRE(!other);
//     auto selected = folder.focus<FocusCamera>("selected");
//     auto hovered = folder.focus<FocusCamera>("hovered");
//     ctx.setFocus(scope, ctx.roles.find("selected"), target);
//     REQUIRE(selected.id() == target);
//     REQUIRE(!hovered);
//     const auto action = folder.on("action", [] {});
//     auto* node = ctx.objects.require(action).node;
//     REQUIRE(node->makeFocusScope() != InvalidFocusScopeId);
//     ctx.setFocus(scope, role, action);
//     REQUIRE(a.id() == action);
//     REQUIRE(!a); // An addressable binding is valid, but is not a Camera.
// }

// TEST(Focus_LocalAndGlobalMerge, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     fixture.blueprints.add<FocusCamera>();
//     root.add<FocusCamera>("one");
//     root.add<FocusCamera>("two");
//     auto one = root.find<FocusCamera>("one").node()->getId();
//     auto two = root.find<FocusCamera>("two").node()->getId();
//     auto& left = root.addFolder("left");
//     auto& right = root.addFolder("right");
//     auto ls = left.makeFocusScope<FocusCamera>(), rs = right.makeFocusScope<FocusCamera>();
//     auto l = left.focus<FocusCamera>(), r = right.focus<FocusCamera>();
//     auto global = ctx.focus<FocusCamera>();
//     auto role = ctx.roles.find(typeKey<FocusCamera>());
//     ctx.setFocus(root.getFocusScopeId(), role, one);
//     ctx.setFocus(rs, role, two);
//     REQUIRE(l.id() == one);
//     REQUIRE(r.id() == two);
//     ctx.activateFocus(ls);
//     REQUIRE(global.id() == one);
//     ctx.setFocus(ls, role, InvalidObjectId);
//     REQUIRE(!l);
//     REQUIRE(!global);
//     ctx.resetFocus(ls, role);
//     REQUIRE(l.id() == one);
//     ctx.activateFocus(rs);
//     REQUIRE(global.id() == two);
//     auto secondary = ctx.getOrCreateRole("secondary");
//     ctx.setFocus(rs, secondary, one);
//     ctx.setFocus(ls, role, two);
//     REQUIRE(global.id() == two);
//     ctx.setFocus(rs, role, one);
//     REQUIRE(global.id() == one);
//     REQUIRE(ctx.resolvedRoles[secondary] == one);
//     REQUIRE(r.id() == one);
//     ctx.resetFocus(rs, role);
//     ctx.setFocus(root.getFocusScopeId(), role, two);
//     REQUIRE(global.id() == two);
//     REQUIRE(ctx.resolvedRoles.size() == ctx.roles.size());
//     REQUIRE(ctx.resolvedRoles[ctx.getOrCreateRole("new")] == InvalidObjectId);
// }

// TEST(Focus_CreateAndSetActivate, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     fixture.blueprints.add<FocusCamera>();
//     root.add<FocusCamera>("one");
//     root.add<FocusCamera>("two");
//     auto one = root.find<FocusCamera>("one").node()->getId();
//     auto two = root.find<FocusCamera>("two").node()->getId();
//     auto& left = root.addFolder("left");
//     auto& right = root.addFolder("right");
//     const auto cameraType = fixture.blueprints.find(typeKey<FocusCamera>());
//     const auto ls = left.makeFocusScope<FocusCamera>();
//     REQUIRE(ctx.activeFocus(cameraType) == ls);
//     const auto rs = right.makeFocusScope<FocusCamera>();
//     REQUIRE(ctx.activeFocus(cameraType) == rs);
//     left.setFocus(typeKey<FocusCamera>(), one);
//     REQUIRE(ctx.activeFocus(cameraType) == ls);
//     REQUIRE(ctx.focus<FocusCamera>().id() == one);
//     right.setFocus(typeKey<FocusCamera>(), two);
//     REQUIRE(ctx.activeFocus(cameraType) == rs);
//     REQUIRE(ctx.focus<FocusCamera>().id() == two);
//     auto& folder = root.addFolder("untyped");
//     folder.makeFocusScope();
//     folder.setFocus("value", folder.getId());
//     REQUIRE(ctx.activeFocus(cameraType) == rs);
//     REQUIRE(ctx.resolveFocus(InvalidFocusScopeId, ctx.roles.find("value")) == InvalidObjectId);
// }

// TEST(Focus_TopologyAndConfigure, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     fixture.blueprints.add<FocusCamera>();
//     fixture.blueprints.add<FocusConsumer>();
//     root.add<FocusCamera>("one");
//     root.add<FocusCamera>("two");
//     auto one = root.find<FocusCamera>("one").node()->getId();
//     auto two = root.find<FocusCamera>("two").node()->getId();
//     auto& middle = root.addFolder("middle");
//     auto& leaf = middle.addFolder("leaf");
//     auto leafScope = leaf.makeFocusScope<FocusCamera>();
//     middle.add<FocusConsumer>();
//     auto consumer = middle.find<FocusConsumer>();
//     consumer->configure(*consumer.node());
//     auto role = ctx.roles.find(typeKey<FocusCamera>());
//     ctx.setFocus(root.getFocusScopeId(), role, one);
//     ctx.activateFocus(leafScope);
//     auto middleScope = middle.makeFocusScope();
//     ctx.setFocus(middleScope, role, two);
//     REQUIRE(ctx.activeScopes == std::vector<FocusScopeId>{leafScope});
//     REQUIRE(ctx.focus<FocusCamera>().id() == two);
//     REQUIRE(consumer->camera.id() == one);
//     consumer->configure(*consumer.node());
//     REQUIRE(consumer->camera.id() == two);
// }

// TEST(Focus_ValidationAndDeletion, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     fixture.blueprints.add<FocusCamera>();
//     fixture.blueprints.add<FocusOwner>();
//     root.add<FocusCamera>();
//     const auto target = root.find<FocusCamera>().node()->getId();
//     root.add<FocusOwner>();
//     auto* owner = root.find<FocusOwner>().node();
//     owner->add<FocusOwner>();
//     auto* child = owner->find<FocusOwner>().node();
//     auto parentScope = owner->makeFocusScope();
//     auto childScope = child->makeFocusScope<FocusCamera>();
//     auto handle = ctx.focus<FocusCamera>();
//     auto role = ctx.roles.find(typeKey<FocusCamera>());
//     ctx.setFocus(parentScope, role, target);
//     ctx.activateFocus(childScope);
//     REQUIRE(throwsFocus([&] { ctx.setFocus(parentScope, role, 999999); }));
//     REQUIRE(throwsFocus([&] { ctx.setFocus(InvalidFocusScopeId, role, target); }));
//     REQUIRE(throwsFocus([&] { ctx.setFocus(parentScope, InvalidRoleId, target); }));
//     REQUIRE(throwsFocus([&] { ctx.resetFocus(parentScope, InvalidRoleId); }));
//     REQUIRE(throwsFocus([&] { ctx.activateFocus(InvalidFocusScopeId); }));
//     REQUIRE(handle.id() == target);
//     owner->remove<FocusOwner>();
//     REQUIRE(ctx.activeFocus(fixture.blueprints.find(typeKey<FocusOwner>())) == parentScope);
//     REQUIRE(!ctx.focusScopes.get(childScope));
//     root.remove<FocusCamera>();
//     REQUIRE(!handle);
//     REQUIRE(ctx.focusScopes.require(parentScope).roles.front().target == InvalidObjectId);
//     root.add<FocusCamera>();
//     REQUIRE(root.find<FocusCamera>().node()->getId() == target);
//     REQUIRE(!handle);
//     ctx.setFocus(root.getFocusScopeId(), role, target);
//     REQUIRE(!handle); // Deletion left Empty, not Unset.
//     root.remove<FocusOwner>();
//     REQUIRE(ctx.activeScopes.empty());
//     REQUIRE(handle.id() == target);
// }

// TEST(Focus_SlotReplacement, RuntimeFixture) {
//     auto& root = fixture.root;
//     auto& ctx = fixture.run_ctx;
//     fixture.blueprints.add<FocusCamera>();
//     auto slot = root.slot<FocusCamera>();
//     auto handle = root.focus<FocusCamera>();
//     auto role = ctx.roles.find(typeKey<FocusCamera>());
//     slot.focus();
//     REQUIRE(!handle);
//     slot.use(typeKey<FocusCamera>());
//     REQUIRE(handle.get() == slot.get());
//     slot->value = 99;
//     slot.use(typeKey<FocusCamera>());
//     REQUIRE(handle.get() == slot.get());
//     REQUIRE(handle->value == 7);
// }
// }

// namespace Lattice {
// namespace {
// struct UniverseScope {};
// struct ViewScope {};
// struct ToolScope {};
// }

// TEST(Focus_IndependentTypedSelections, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     auto& root = fixture.root;
//     fixture.blueprints.add<UniverseScope>();
//     fixture.blueprints.add<ViewScope>();
//     fixture.blueprints.add<ToolScope>();
//     auto& u1 = root.addFolder("universe1");
//     auto& u2 = root.addFolder("universe2");
//     auto& v1 = root.addFolder("viewport1");
//     auto& v2 = root.addFolder("viewport2");
//     const auto us1 = u1.makeFocusScope<UniverseScope>();
//     const auto us2 = u2.makeFocusScope<UniverseScope>();
//     const auto vs1 = v1.makeFocusScope<ViewScope>();
//     const auto vs2 = v2.makeFocusScope<ViewScope>();
//     const auto data1 = u1.addFolder("data").getId();
//     const auto data2 = u2.addFolder("data").getId();
//     u1.setFocus("data", data1);
//     u2.setFocus("data", data2);
//     v1.setFocus("camera", v1.getId());
//     v2.setFocus("camera", v2.getId());
//     u1.setFocus("shared", data1);
//     v1.setFocus("shared", v1.getId());
//     fixture.root.setFocus("fallback", fixture.root.getId());
//     u1.setFocus("fallback", data1);
//     u1.setFocus("old-only", data1);
//     auto& tool = u1.addFolder("tool");
//     const auto ts = tool.makeFocusScope<ToolScope>();
//     tool.setFocus("tool", tool.getId());
//     const auto read = [&](std::string_view name) {
//         return ctx.resolveFocus(InvalidFocusScopeId, ctx.roles.find(name));
//     };
//     ctx.activateFocus(ts); // Also selects the enclosing Universe.
//     ctx.activateFocus(vs1);
//     REQUIRE(read("data") == data1);
//     REQUIRE(read("shared") == v1.getId());
//     REQUIRE(read("fallback") == data1);
//     ctx.activateFocus(us1); // Raise this branch, retaining its active tool.
//     REQUIRE(read("shared") == data1);
//     REQUIRE(read("tool") == tool.getId());
//     ctx.activateFocus(vs1);
//     const auto select = root.on("selectUniverse2", [&ctx, us2] { ctx.activateFocus(us2); });
//     ctx.bindings.invoke(select);
//     REQUIRE(read("data") == data2);
//     REQUIRE(read("camera") == v1.getId());
//     REQUIRE(read("old-only") == InvalidObjectId);
//     REQUIRE(read("fallback") == fixture.root.getId());
//     REQUIRE(read("tool") == InvalidObjectId);
//     REQUIRE(ctx.activeFocus(fixture.blueprints.find(typeKey<ToolScope>())) == InvalidFocusScopeId);
//     REQUIRE(ctx.resolveFocus(ts, ctx.roles.find("tool")) == tool.getId());
//     REQUIRE(ctx.focusScopes.get(us1));
//     ctx.activateFocus(vs2);
//     REQUIRE(read("data") == data2);
//     REQUIRE(read("camera") == v2.getId());
//     ctx.activateFocus(us1);
//     REQUIRE(read("data") == data1);
//     REQUIRE(read("tool") == InvalidObjectId); // No implicit restoration of old tools.
//     REQUIRE(read("camera") == v2.getId());
//     ctx.activateFocus(root.getFocusScopeId());
//     REQUIRE(read("camera") == v2.getId());
// }

// TEST(Focus_UntypedAndInvalidActivation, RuntimeFixture) {
//     auto& ctx = fixture.run_ctx;
//     fixture.blueprints.add<UniverseScope>();
//     auto& folder = fixture.root.addFolder("untyped");
//     const auto untyped = folder.makeFocusScope();
//     folder.setFocus("value", folder.getId());
//     REQUIRE(ctx.resolveFocus(untyped, ctx.roles.find("value")) == folder.getId());
//     REQUIRE(throwsFocus([&] { ctx.activateFocus(untyped); }));
//     auto& parent = fixture.root.addFolder("parent");
//     const auto ps = parent.makeFocusScope<UniverseScope>();
//     auto& child = parent.addFolder("child");
//     const auto cs = child.makeFocusScope<UniverseScope>();
//     ctx.activateFocus(ps);
//     REQUIRE(throwsFocus([&] { ctx.activateFocus(cs); }));
//     REQUIRE(ctx.activeFocus(fixture.blueprints.find(typeKey<UniverseScope>())) == ps);
//     REQUIRE(throwsFocus([&] { parent.makeFocusScope<Model>(); }));
// }
// }
