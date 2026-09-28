// #include <Lattice/Tools/Fixture.hpp>
// #include <Lattice/Tools/Tests.hpp>

// namespace Lattice {
// namespace {
// struct Description { int size = 7; };
// struct OtherDescription { int size = 9; };
// struct ConvertibleDescription { operator Description() const { return {}; } };
// struct DescribedAPI { virtual ~DescribedAPI() = default; };
// struct DescribedComponent : DescribedAPI {
//     using Desc = Description;
//     int size;
//     DescribedComponent(Node&, const Desc& desc) : size(desc.size) {}
// };
// struct PlainComponent {};

// template<typename T, typename D>
// concept CanAdd = requires(Node& node, D&& desc) { node.add<T>("instance", std::forward<D>(desc)); };
// template<typename T, typename D>
// concept CanUse = requires(Node& node, D&& desc) { node.use<DescribedAPI, T>("instance", std::forward<D>(desc)); };
// template<typename D>
// concept CanAddByName = requires(Node& node, D desc) { node.add("DescribedComponent", "instance", desc); };

// static_assert(CanAdd<DescribedComponent, Description>);
// static_assert(CanAdd<DescribedComponent, const Description&>);
// static_assert(!CanAdd<DescribedComponent, OtherDescription>);
// static_assert(!CanAdd<DescribedComponent, ConvertibleDescription>);
// static_assert(!CanAdd<DescribedComponent, Description*>);
// static_assert(!CanAdd<PlainComponent, Description>);
// static_assert(CanUse<DescribedComponent, Description>);
// static_assert(CanUse<DescribedComponent, const Description&>);
// static_assert(!CanUse<DescribedComponent, OtherDescription>);
// static_assert(!CanUse<DescribedComponent, ConvertibleDescription>);
// static_assert(!CanUse<PlainComponent, Description>);
// static_assert(!CanAddByName<const void*>);

// struct Owner;
// struct Owned {
//     Owner* owner;
//     explicit Owned(Node& node) : owner(node.requireParent<Owner>().getPtr()) {}
// };
// struct Owner {
//     void configure(Node& node) { node.add<Owned>(); }
// };
// }

// TEST(Descriptor_AddAndUse, RuntimeFixture) {
//     fixture.blueprints.add<DescribedAPI>();
//     fixture.blueprints.add<DescribedComponent, DescribedAPI>();
//     const Description desc{42};
//     auto& custom = *fixture.root.add<DescribedComponent>("custom", desc);
//     REQUIRE(custom.size == 42);
//     REQUIRE(fixture.root.add<DescribedComponent>("defaulted")->size == 7);
//     fixture.root.slot<DescribedAPI>("slot");
//     fixture.root.use<DescribedAPI, DescribedComponent>("slot", Description{83});
//     auto slot = fixture.root.find<DescribedAPI>("slot");
//     REQUIRE(static_cast<DescribedComponent*>(slot.get())->size == 83);
//     fixture.root.use<DescribedAPI, DescribedComponent>("slot", desc);
//     REQUIRE(static_cast<DescribedComponent*>(slot.get())->size == 42);
//     slot.use("DescribedComponent");
//     REQUIRE(static_cast<DescribedComponent*>(slot.get())->size == 7);
// }

// TEST(Node_RequireParentDuringConfigure, RuntimeFixture) {
//     fixture.blueprints.add<Owner>();
//     fixture.blueprints.add<Owned>();
//     auto& first = *fixture.root.add<Owner>("first");
//     auto& second = *fixture.root.add<Owner>("second");
//     fixture.root.configureBranch();
//     auto& firstNode = fixture.root.require("Owner", "first");
//     auto& secondNode = fixture.root.require("Owner", "second");
//     REQUIRE(firstNode.require<Owned>()->owner == &first);
//     REQUIRE(secondNode.require<Owned>()->owner == &second);
//     // Reconfiguring must not duplicate the device-like child.
//     first.configure(firstNode);
//     REQUIRE(firstNode.directCollect<Owned>().size() == 1);
//     bool rejected = false;
//     try { fixture.root.requireParent<Owner>(); }
//     catch (const ExceptionBase&) { rejected = true; }
//     REQUIRE(rejected);
//     auto& folder = fixture.root.addFolder("unrelated");
//     rejected = false;
//     try { folder.requireParent<Owner>(); }
//     catch (const ExceptionBase&) { rejected = true; }
//     REQUIRE(rejected);
// }

// namespace {
// struct DescriptorAPI {
//     using Desc = Description;
//     virtual ~DescriptorAPI() = default;
// };
// struct DescriptorImpl : DescriptorAPI {
//     int size;
//     DescriptorImpl(Node&, const Desc& desc) : size(desc.size) {}
// };
// struct AbstractDescriptorAPI {
//     using Desc = Description;
//     virtual ~AbstractDescriptorAPI() = default;
//     virtual void operation() = 0;
// };
// }

// TEST(Descriptor_APIWithoutFactory, RuntimeFixture) {
//     const auto api = fixture.blueprints.add<DescriptorAPI>();
//     const auto abstractApi = fixture.blueprints.add<AbstractDescriptorAPI>();
//     const auto impl = fixture.blueprints.add<DescriptorImpl, DescriptorAPI>();
//     REQUIRE(!fixture.blueprints.require(api).meta.create);
//     REQUIRE(!fixture.blueprints.require(abstractApi).meta.create);
//     REQUIRE(fixture.blueprints.require(impl).meta.create);
//     auto slot = fixture.root.slot<DescriptorAPI>();
//     fixture.root.use<DescriptorAPI, DescriptorImpl>("", Description{42});
//     REQUIRE(static_cast<DescriptorImpl*>(slot.get())->size == 42);
//     REQUIRE(fixture.root.add<DescriptorImpl>("defaulted")->size == 7);
//     auto created = fixture.root.add<DescriptorAPI>("viaAPI", Description{53});
//     REQUIRE(static_cast<DescriptorImpl*>(created.getPtr())->size == 53);
// }

// }
