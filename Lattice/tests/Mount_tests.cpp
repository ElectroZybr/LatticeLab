#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice::MountTests {
struct Prefix { virtual ~Prefix() = default; int padding = 17; };
struct API { virtual ~API() = default; virtual int value() const = 0; };
struct Impl : Prefix, API {
    static inline int destroyed = 0;
    ~Impl() override { ++destroyed; }
    int value() const override { return 42; }
};
struct Other : API { int value() const override { return 99; } };

TEST(Mount_AbstractAPI, RuntimeFixture) {
    fixture.blueprints.add<API>();
    fixture.blueprints.add<Impl, API>();
    // Ensure role IDs and object IDs do not accidentally coincide.
    fixture.run_ctx.getOrCreateRole("unused");
    fixture.run_ctx.getOrCreateRole("alsoUnused");
    auto component = fixture.root.add<Impl>();
    const auto id = fixture.root.find<Impl>().node->getId();
    fixture.root.setFocus(typeKey<API>(), id);
    fixture.root.setFocus(typeKey<Impl>(), id);
    auto& consumer = fixture.root.addFolder("consumer");
    Impl::destroyed = 0;
    auto mounted = consumer.mount<API>();
    REQUIRE(mounted.getPtr() == static_cast<API*>(component.getPtr()));
    REQUIRE(mounted->value() == 42);
    REQUIRE(consumer.require<API>("API").getPtr() == mounted.getPtr());
    consumer.configureBranch();
    consumer.remove<API>("API");
    REQUIRE(Impl::destroyed == 0);
    REQUIRE(component->value() == 42);
    REQUIRE(consumer.mount<Impl>().getPtr() == component.getPtr());
}

TEST(Mount_ActiveSelection, RuntimeFixture) {
    fixture.blueprints.add<API>();
    fixture.blueprints.add<Impl, API>();
    fixture.blueprints.add<Other, API>();
    auto& consumer = fixture.root.addFolder("consumer");
    bool rejected = false;
    try { consumer.mount<API>(); } catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(consumer.collectTree().empty());
    fixture.root.add<Impl>();
    auto other = fixture.root.add<Other>();
    rejected = false;
    try { consumer.mount<API>(); } catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(consumer.collectTree().empty());
    auto& target = fixture.root.require("Other");
    fixture.root.setFocus(typeKey<API>(), target.getId());
    REQUIRE(consumer.mount<API>().getPtr() == static_cast<API*>(other.getPtr()));
}

TEST(Mount_SlotImplementation, RuntimeFixture) {
    fixture.blueprints.add<API>();
    fixture.blueprints.add<Impl, API>();
    auto slot = fixture.root.slot<API>();
    fixture.root.use<API, Impl>();
    fixture.root.setFocus(typeKey<API>(), slot.node->getId());
    auto& consumer = fixture.root.addFolder("consumer");
    REQUIRE(consumer.mount<API>().getPtr() == slot.get());
}
}

namespace Lattice::MountTests {
TEST(Mount_LocalRoleAndSnapshot, RuntimeFixture) {
    fixture.blueprints.add<API>();
    fixture.blueprints.add<Impl, API>();
    fixture.blueprints.add<Other, API>();
    fixture.root.add<Impl>();
    fixture.root.add<Other>();
    auto* first = fixture.root.find<Impl>().node;
    auto* second = fixture.root.find<Other>().node;
    fixture.root.setFocus(typeKey<API>(), first->getId());
    auto& consumer = fixture.root.addFolder("local");
    consumer.makeFocusScope();
    consumer.setFocus("device", second->getId());
    auto mounted = consumer.mount<API>("device");
    REQUIRE(&mounted.branch() == second);
    REQUIRE(mounted->value() == 99);
    consumer.setFocus("device", first->getId());
    REQUIRE(&mounted.branch() == second);
    REQUIRE(mounted->value() == 99);
    consumer.setFocus(typeKey<API>(), InvalidObjectId);
    bool rejected = false;
    try { consumer.mount<API>(); } catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    consumer.setFocus("bad", consumer.getId());
    rejected = false;
    try { consumer.mount<API>("bad"); } catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
}
}
