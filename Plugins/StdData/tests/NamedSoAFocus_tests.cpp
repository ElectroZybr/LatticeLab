#include <array>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include "NamedSoALoader.hpp"

namespace {
struct UniverseScope {};
struct ViewportScope {};
struct Element { using type = std::array<char, 8>; };
struct Mass { using type = float; };
}

TEST(NamedSoA_SelectedUniverseSurvivesViewportActivation, Lattice::RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    fixture.blueprints.add<UniverseScope>();
    fixture.blueprints.add<ViewportScope>();
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<StdData::NamedSoA, StdData::SoA>();
    auto& first = fixture.root.addFolder("Universe1");
    auto& second = fixture.root.addFolder("Universe2");
    auto& viewport = fixture.root.addFolder("Viewport");
    const auto firstScope = first.makeFocusScope<UniverseScope>();
    const auto secondScope = second.makeFocusScope<UniverseScope>();
    const auto viewScope = viewport.makeFocusScope<ViewportScope>();
    auto a = first.add<StdData::NamedSoA>();
    auto b = second.add<StdData::NamedSoA>();
    a->addCol<Element>(); a->addCol<Mass>();
    b->addCol<Element>(); b->addCol<Mass>();
    first.find<StdData::NamedSoA>().focus("AtomData");
    second.find<StdData::NamedSoA>().focus("AtomData");
    viewport.setFocus("camera", viewport.getId());
    NamedSoALoader loader;
    loader.configure(fixture.root);
    const Lattice::Value data = Lattice::Object{
        {"target", std::string("AtomData")},
        {"columns", Lattice::Array{std::string("Element"), std::string("Mass")}},
        {"rows", Lattice::Array{Lattice::Array{std::string("H"), 1.008}}}
    };
    const auto load = fixture.root.on("load", [&] { loader.load(data); });
    const auto select = fixture.root.on("selectUniverse2", [&] { ctx.activateFocus(secondScope); });
    ctx.activateFocus(firstScope);
    ctx.activateFocus(viewScope);
    ctx.bindings.invoke(load);
    REQUIRE(a->size() == 1);
    REQUIRE(b->size() == 0);
    REQUIRE(static_cast<const float*>(a->get("Mass"))[0] > 1.0f);
    ctx.bindings.invoke(select);
    REQUIRE(ctx.activeFocus(fixture.blueprints.find(Lattice::typeKey<ViewportScope>())) == viewScope);
    ctx.bindings.invoke(load);
    REQUIRE(a->size() == 1);
    REQUIRE(b->size() == 1);
    REQUIRE(static_cast<const float*>(b->get("Mass"))[0] > 1.0f);
}
