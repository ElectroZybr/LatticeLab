#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice::BlueprintRuntimeTests {
struct Left { virtual ~Left() = default; int left = 11; };
struct Right { virtual ~Right() = default; int right = 22; };
struct Both : Left, Right {
    static inline int destroyed = 0;
    ~Both() override { ++destroyed; }
};
struct Root { int value = 7; };
struct VLeft : virtual Root {};
struct VRight : virtual Root {};
struct VDiamond : VLeft, VRight {};
struct NLeft : Root {};
struct NRight : Root {};
struct NDiamond : NLeft, NRight {};
namespace First { struct Device {}; }
namespace Second { struct Device {}; }
struct Throws {
    explicit Throws(Node& node) {
        node.add<Left>();
        throw Exception("Test", "constructor failure");
    }
};

TEST(BlueprintRuntime_MultipleInterfaces, RuntimeFixture) {
    auto& types = fixture.blueprints;
    types.add<Left>();
    types.add<Right>();
    types.add<Both, Left, Right>();
    const auto before = fixture.root.collectTree().size();
    Both::destroyed = 0;
    auto& both = *fixture.root.add<Both>("device");
    REQUIRE(fixture.root.collectTree().size() == before + 1);
    REQUIRE(fixture.root.require<Left>("device").getPtr() == static_cast<Left*>(&both));
    REQUIRE(fixture.root.require<Right>("device").getPtr() == static_cast<Right*>(&both));
    REQUIRE(fixture.root.directCollect<Right>()[0]->right == 22);
    fixture.root.remove<Right>("device");
    REQUIRE(Both::destroyed == 1);
    auto slot = fixture.root.slot<Right>();
    fixture.root.use<Right, Both>();
    REQUIRE(slot->right == 22);
    REQUIRE(fixture.root.require<Left>()->left == 11);
    REQUIRE(fixture.root.globalCollect<Both>().size() == 1);
    slot.use<Both>();
    REQUIRE(slot->right == 22);
    REQUIRE(Both::destroyed == 2);
}

TEST(BlueprintRuntime_Diamonds, RuntimeFixture) {
    auto& types = fixture.blueprints;
    types.add<Root>();
    types.add<VLeft, Root>(); types.add<VRight, Root>();
    types.add<VDiamond, VLeft, VRight>();
    types.add<NLeft, Root>(); types.add<NRight, Root>();
    types.add<NDiamond, NLeft, NRight>();
    auto& virtualDiamond = *fixture.root.add<VDiamond>("virtual");
    REQUIRE(fixture.root.require<Root>("virtual").getPtr() == static_cast<Root*>(&virtualDiamond));
    fixture.root.add<NDiamond>("nonvirtual");
    bool ambiguous = false;
    try { fixture.root.require<Root>("nonvirtual"); }
    catch (const Exception&) { ambiguous = true; }
    REQUIRE(ambiguous);
    auto slot = fixture.root.slot<Root>("slot");
    ambiguous = false;
    try { slot.use("NDiamond"); }
    catch (const Exception&) { ambiguous = true; }
    REQUIRE(ambiguous);
    REQUIRE(!slot.exists());
}

TEST(BlueprintRuntime_NamesAndFailedConstruction, RuntimeFixture) {
    auto& types = fixture.blueprints;
    types.add<First::Device>(); types.add<Second::Device>();
    fixture.root.add<First::Device>(); fixture.root.add<Second::Device>();
    REQUIRE(fixture.root.require<First::Device>());
    REQUIRE(fixture.root.require<Second::Device>());
    bool ambiguous = false;
    try { fixture.root.addNode("Device", ""); }
    catch (const Exception&) { ambiguous = true; }
    REQUIRE(ambiguous);
    types.add<Left>(); types.add<Throws>();
    const auto before = fixture.root.collectTree().size();
    bool rejected = false;
    try { fixture.root.add<Throws>(); }
    catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(fixture.root.collectTree().size() == before);
}
}

namespace Lattice::CreationTests {
struct API {
    struct Desc { int value = 7; };
    virtual ~API() = default;
    virtual int value() const = 0;
};
struct Prefix { virtual ~Prefix() = default; int padding = 99; };
namespace Backend {
struct Device {};
struct Buffer : Prefix, API {
    int stored;
    Buffer(Node&, const Desc& desc) : stored(desc.value) {}
    int value() const override { return stored; }
};
}
namespace Other {
struct Buffer : API {
    Buffer(Node&, const Desc&) {}
    int value() const override { return -1; }
};
struct WrongDescriptor : API {
    struct Desc { double value = 0; };
    WrongDescriptor(Node&, const Desc&) {}
    int value() const override { return -2; }
};
}
TEST(BlueprintRuntime_CreateThroughAPI, RuntimeFixture) {
    fixture.blueprints.add<API>();
    fixture.blueprints.add<Other::WrongDescriptor, API>();
    bool rejected = false;
    try { fixture.root.add<API>("wrong", API::Desc{42}); }
    catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    fixture.blueprints.add<Backend::Device>();
    fixture.blueprints.add<Backend::Buffer, API>();
    auto created = fixture.root.add<API>("unique", API::Desc{42});
    REQUIRE(created->value() == 42);
    REQUIRE(created.getPtr() == static_cast<API*>(fixture.root.require<Backend::Buffer>("unique").getPtr()));
    fixture.blueprints.add<Other::Buffer, API>();
    rejected = false;
    try { fixture.root.add<API>("ambiguous", API::Desc{}); }
    catch (const Exception&) { rejected = true; }
    REQUIRE(rejected);
    auto& device = fixture.root.addNode(typeKey<Backend::Device>(), "device");
    REQUIRE(device.add<API>("local", API::Desc{83})->value() == 83);
}
}
