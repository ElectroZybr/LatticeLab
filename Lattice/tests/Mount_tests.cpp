// #include <Lattice/Tools/Fixture.hpp>
// #include <Lattice/Tools/Tests.hpp>

// namespace Lattice::MountTests {
// struct Prefix { virtual ~Prefix() = default; int padding = 17; };
// struct API { virtual ~API() = default; virtual int value() const = 0; };
// struct Impl : Prefix, API {
//     static inline int destroyed = 0;
//     ~Impl() override { ++destroyed; }
//     int value() const override { return 42; }
// };
// struct Other : API { int value() const override { return 99; } };
// struct Child {
//     int value = 0;
//     explicit Child(Node&) {}
// };
// struct ChildDesc { int value = 0; };
// struct DescribedChild {
//     using Desc = ChildDesc;
//     int value;
//     DescribedChild(Node&, const Desc& desc) : value(desc.value) {}
// };
// struct ChildNeedsParent {
//     API* api;
//     Impl* impl;
//     explicit ChildNeedsParent(Node& node)
//         : api(node.requireParent<API>().getPtr())
//         , impl(node.requireParent<Impl>().getPtr()) {}
// };

// TEST(Mount_AbstractAPI, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     // Ensure role IDs and object IDs do not accidentally coincide.
//     fixture.run_ctx.getOrCreateRole("unused");
//     fixture.run_ctx.getOrCreateRole("alsoUnused");
//     auto component = fixture.root.add<Impl>();
//     const auto id = fixture.root.find<Impl>().node()->getId();
//     fixture.root.setFocus(typeKey<API>(), id);
//     fixture.root.setFocus(typeKey<Impl>(), id);
//     auto& consumer = fixture.root.addFolder("consumer");
//     Impl::destroyed = 0;
//     auto mounted = consumer.mount<API>();
//     REQUIRE(mounted.getPtr() == static_cast<API*>(component.getPtr()));
//     REQUIRE(mounted->value() == 42);
//     REQUIRE(mounted.node() != component.node());
//     REQUIRE(mounted.node()->getParent() == &consumer);
//     REQUIRE(mounted.node()->getKind() == NodeKind::Mount);
//     REQUIRE(consumer.require<API>().getPtr() == mounted.getPtr());
//     REQUIRE(consumer.require<API>().node() == mounted.node());
//     consumer.configureBranch();
//     consumer.remove<API>();
//     REQUIRE(Impl::destroyed == 0);
//     REQUIRE(component->value() == 42);
//     REQUIRE(consumer.mount<Impl>().getPtr() == component.getPtr());
// }

// TEST(Mount_ActiveSelection, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     fixture.blueprints.add<Other, API>();
//     auto& consumer = fixture.root.addFolder("consumer");
//     bool rejected = false;
//     try { consumer.mount<API>(); } catch (const Exception&) { rejected = true; }
//     REQUIRE(rejected);
//     REQUIRE(consumer.collectTree().empty());
//     fixture.root.add<Impl>();
//     auto other = fixture.root.add<Other>();
//     rejected = false;
//     try { consumer.mount<API>(); } catch (const Exception&) { rejected = true; }
//     REQUIRE(rejected);
//     REQUIRE(consumer.collectTree().empty());
//     auto& target = fixture.root.require("Other");
//     fixture.root.setFocus(typeKey<API>(), target.getId());
//     REQUIRE(consumer.mount<API>().getPtr() == static_cast<API*>(other.getPtr()));
// }

// TEST(Mount_SlotImplementation, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     auto slot = fixture.root.slot<API>();
//     fixture.root.use<API, Impl>();
//     fixture.root.setFocus(typeKey<API>(), slot.node()->getId());
//     auto& consumer = fixture.root.addFolder("consumer");
//     REQUIRE(consumer.mount<API>().getPtr() == slot.get());
// }
// }

// namespace Lattice::MountTests {
// TEST(Mount_LocalRoleAndSnapshot, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     fixture.blueprints.add<Other, API>();
//     fixture.root.add<Impl>();
//     fixture.root.add<Other>();
//     auto* first = fixture.root.find<Impl>().node();
//     auto* second = fixture.root.find<Other>().node();
//     fixture.root.setFocus(typeKey<API>(), first->getId());
//     auto& consumer = fixture.root.addFolder("local");
//     consumer.makeFocusScope();
//     consumer.setFocus("device", second->getId());
//     auto mounted = consumer.mount<API>("device");
//     REQUIRE(mounted.node() != second);
//     REQUIRE(mounted.node()->getParent() == &consumer);
//     REQUIRE(mounted.getPtr() == static_cast<API*>(second->get<API>()));
//     REQUIRE(mounted->value() == 99);
//     consumer.setFocus("device", first->getId());
//     REQUIRE(mounted.getPtr() == static_cast<API*>(second->get<API>()));
//     REQUIRE(mounted->value() == 99);
//     consumer.setFocus(typeKey<API>(), InvalidObjectId);
//     REQUIRE(consumer.mount<API>().node() == mounted.node());
//     auto& other = fixture.root.addFolder("other");
//     other.makeFocusScope();
//     other.setFocus(typeKey<API>(), InvalidObjectId);
//     bool rejected = false;
//     try { other.mount<API>(); } catch (const Exception&) { rejected = true; }
//     REQUIRE(rejected);
//     other.setFocus("bad", other.getId());
//     rejected = false;
//     try { other.mount<API>("bad"); } catch (const Exception&) { rejected = true; }
//     REQUIRE(rejected);
// }

// TEST(Mount_UnnamedRequireFromChild, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     auto component = fixture.root.add<Impl>();
//     fixture.root.setFocus(typeKey<API>(), component.node()->getId());
//     auto& consumer = fixture.root.addFolder("consumer");
//     auto mounted = consumer.mount<API>();
//     auto& view = consumer.addFolder("view");
//     auto found = view.require<API>();
//     REQUIRE(found.getPtr() == mounted.getPtr());
//     REQUIRE(found.node() == mounted.node());
//     REQUIRE(consumer.mount<API>().node() == mounted.node());
// }

// TEST(Mount_WorkspaceIsLocal, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     fixture.blueprints.add<Child>();
//     fixture.blueprints.add<DescribedChild>();
//     auto host = fixture.root.add<Impl>();
//     fixture.root.setFocus(typeKey<API>(), host.node()->getId());
//     auto& consumer = fixture.root.addFolder("consumer");
//     auto mounted = consumer.mount<API>();

//     auto child = mounted.add<Child>("pipe");
//     REQUIRE(child.exists());
//     REQUIRE(child.node()->getParent() == mounted.node());
//     REQUIRE(mounted.children<Child>().size() == 1);
//     REQUIRE(host.node()->directCollect<Child>().empty());
//     REQUIRE(consumer.directCollect<Child>().empty());

//     auto described = mounted.add<DescribedChild>("described", ChildDesc{11});
//     REQUIRE(described->value == 11);
//     REQUIRE(described.node()->getParent() == mounted.node());

//     int value = 4;
//     int seen = 0;
//     const auto bound = mounted.bind("gain", &value, [&](int next) { seen = next; });
//     REQUIRE(fixture.run_ctx.objects.require(bound).node->getParent() == mounted.node());
//     fixture.run_ctx.bindings.set(bound, 8);
//     REQUIRE(value == 8);
//     REQUIRE(seen == 8);

//     int calls = 0;
//     const auto action = mounted.on("rebuild", [&] { ++calls; });
//     REQUIRE(fixture.run_ctx.objects.require(action).node->getParent() == mounted.node());
//     fixture.run_ctx.bindings.invoke(action);
//     REQUIRE(calls == 1);

//     auto& view = consumer.addFolder("view");
//     view.require<API>().add<Child>("fromChild");
//     REQUIRE(mounted.children<Child>().size() == 2);
//     REQUIRE(host.node()->directCollect<Child>().empty());

//     mounted.remove<Child>("pipe");
//     REQUIRE(mounted.find<Child>("fromChild").exists());
//     REQUIRE(!mounted.find<Child>("pipe").exists());
// }

// TEST(Mount_RequireParentFromWorkspace, RuntimeFixture) {
//     fixture.blueprints.add<API>();
//     fixture.blueprints.add<Impl, API>();
//     fixture.blueprints.add<ChildNeedsParent>();
//     auto host = fixture.root.add<Impl>();
//     fixture.root.setFocus(typeKey<API>(), host.node()->getId());
//     auto& consumer = fixture.root.addFolder("consumer");
//     auto mounted = consumer.mount<API>();
//     auto child = mounted.add<ChildNeedsParent>();
//     REQUIRE(child->api == static_cast<API*>(host.getPtr()));
//     REQUIRE(child->impl == host.getPtr());
//     REQUIRE(child.node()->getParent() == mounted.node());
// }
// }
