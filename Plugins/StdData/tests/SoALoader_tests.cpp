#include <array>
#include <cstring>

#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>

#include "Document.hpp"
#include "SoA.hpp"
#include "SoALoader.hpp"
#include "TomlParser.hpp"

using Lattice::RuntimeFixture;

namespace {

struct Name { using type = std::array<char, 8>; };
struct Mass { using type = float; };
struct Valence { using type = uint8_t; };

class Wrapper {
public:
    explicit Wrapper(Lattice::Node& branch) {
        branch.add<StdData::SoA>();
        soa = branch.require<StdData::SoA>();
        soa->addCol<Name>();
        soa->addCol<Mass>();
        soa->addCol<Valence>();
    }

    Ref<StdData::SoA> soa;
};

class AtomData {
public:
    explicit AtomData(Lattice::Node& branch) {
        branch.add<StdData::SoA>();
        soa = branch.require<StdData::SoA>();
        soa->addCol<Name>();
        soa->addCol<Mass>();
        soa->addCol<Valence>();
    }

    Ref<StdData::SoA> soa;
};

Document makeSoADocument(std::string target) {
    Document document;
    document.root().emplace("SoAData", Lattice::Table{{"target", std::move(target)}, {"columns", Lattice::Array{std::string("Mass"), std::string("Valence")}}, {"rows", Lattice::Array{Lattice::Array{Lattice::Value{1.008}, Lattice::Value{int64_t{1}}}, Lattice::Array{Lattice::Value{4.003}, Lattice::Value{int64_t{0}}}}}});
    return document;
}

} // namespace

TEST(SoALoader_LoadIntoWrapper, RuntimeFixture,
    "Лоадер должен найти SoA под активной обёрткой в контексте и записать строки.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<Wrapper>();
    fixture.root.add<Wrapper>();
    fixture.root.setFocus("Wrapper", fixture.root.find<Wrapper>().node->getId());

    SoALoader loader;
    loader.configure(fixture.root);

    const Document document = makeSoADocument("Wrapper");
    const Lattice::Value* section = document.section(loader.section());
    if (section)
        loader.load(*section);

    auto soa = fixture.root.require<Wrapper>("default")->soa;

    REQUIRE(soa);
    REQUIRE(soa->size() == 2);
    REQUIRE(soa->at<Mass>(0) == static_cast<float>(1.008));
    REQUIRE(soa->at<Valence>(0) == 1);
    REQUIRE(soa->at<Mass>(1) == static_cast<float>(4.003));
    REQUIRE(soa->at<Valence>(1) == 0);
}

TEST(SoALoader_LoadNames, RuntimeFixture,
    "Строковые значения должны записываться в char-колонку SoA.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<Wrapper>();
    fixture.root.add<Wrapper>();
    fixture.root.setFocus("Wrapper", fixture.root.find<Wrapper>().node->getId());

    SoALoader loader;
    loader.configure(fixture.root);

    Document document;
    document.root().emplace("SoAData", Lattice::Table{{"target", std::string("Wrapper")}, {"columns", Lattice::Array{std::string("Name"), std::string("Mass"), std::string("Valence")}}, {"rows", Lattice::Array{Lattice::Array{std::string("H"), Lattice::Value{1.008}, Lattice::Value{int64_t{1}}}, Lattice::Array{std::string("He"), Lattice::Value{4.003}, Lattice::Value{int64_t{0}}}}}});
    const Lattice::Value* section = document.section(loader.section());
    if (section)
        loader.load(*section);

    auto soa = fixture.root.require<Wrapper>("default")->soa;
    REQUIRE(soa->size() == 2);
    REQUIRE(std::strcmp(soa->at<Name>(0).data(), "H") == 0);
    REQUIRE(std::strcmp(soa->at<Name>(1).data(), "He") == 0);
}

TEST(SoALoader_MissingTarget, RuntimeFixture,
    "Лоадер должен падать, если цель не активна в контексте.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<Wrapper>();
    fixture.root.add<Wrapper>();
    fixture.root.setFocus("Wrapper", fixture.root.find<Wrapper>().node->getId());

    SoALoader loader;
    loader.configure(fixture.root);

    bool thrown = false;
    try {
        const Document document = makeSoADocument("AtomData");
        const Lattice::Value* section = document.section(loader.section());
        if (section)
            loader.load(*section);
    } catch (const Lattice::Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(SoALoader_MissingColumn, RuntimeFixture,
    "Лоадер должен падать, если колонки нет в буфере.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<Wrapper>();
    fixture.root.add<Wrapper>();
    fixture.root.setFocus("Wrapper", fixture.root.find<Wrapper>().node->getId());

    SoALoader loader;
    loader.configure(fixture.root);

    Document document;
    document.root().emplace("SoAData", Lattice::Table{{"target", std::string("Wrapper")}, {"columns", Lattice::Array{std::string("Charge")}}, {"rows", Lattice::Array{Lattice::Array{Lattice::Value{1.0}}}}});

    bool thrown = false;
    try {
        const Lattice::Value* section = document.section(loader.section());
        if (section)
            loader.load(*section);
    } catch (const Lattice::Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(SoALoader_InspectEmpty, RuntimeFixture,
    "inspect пустого SoA не должен падать.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.root.add<StdData::SoA>();

    auto soa = fixture.root.require<StdData::SoA>();
    REQUIRE(soa->size() == 0);
    soa->inspect("empty");
}

TEST(SoALoader_LoadAtomDataFile, RuntimeFixture,
    "Config/atomData.toml должен заполнить SoA у AtomData.")
{
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<AtomData>();
    fixture.root.add<AtomData>();
    fixture.root.setFocus("AtomData", fixture.root.find<AtomData>().node->getId());

    TomlParser parser;
    const Document doc = parser.parseFile("Config/atomData.toml");
    const Lattice::Value* data = doc.get("SoAData");
    REQUIRE(data);
    REQUIRE(data->is<Lattice::Table>());

    const auto& table = std::get<Lattice::Table>(*data);
    const auto rowsIt = table.find("rows");
    REQUIRE(rowsIt != table.end());
    REQUIRE(rowsIt->second.is<Lattice::Array>());
    REQUIRE(std::get<Lattice::Array>(rowsIt->second)[0].is<Lattice::Array>());

    SoALoader loader;
    loader.configure(fixture.root);
    const Lattice::Value* section = doc.section(loader.section());
    if (section)
        loader.load(*section);

    auto soa = fixture.root.require<AtomData>()->soa;
    REQUIRE(soa);
    REQUIRE(soa->size() == 3);
    REQUIRE(soa->columnCount() == 3);
    REQUIRE(std::strcmp(soa->at<Name>(0).data(), "H") == 0);
    REQUIRE(soa->at<Mass>(0) == static_cast<float>(1.008));
    REQUIRE(soa->at<Valence>(0) == 1);
    REQUIRE(std::strcmp(soa->at<Name>(2).data(), "Li") == 0);

    soa->inspect("AtomData");
}
TEST(SoALoader_ExplicitFocusTarget, RuntimeFixture) {
    fixture.blueprints.add<StdData::SoA>();
    fixture.blueprints.add<Wrapper>();
    fixture.root.add<Wrapper>();
    const auto id = fixture.root.find<Wrapper>().node->getId();
    SoALoader loader;
    loader.configure(fixture.root);
    // Empty rows isolate target resolution from the unfinished row writer.
    const Lattice::Value data = Lattice::Table{
        {"target", std::string("data")},
        {"columns", Lattice::Array{std::string("Mass")}},
        {"rows", Lattice::Array{}}
    };
    bool rejected = false;
    try { loader.load(data); } catch (const Lattice::Exception&) { rejected = true; }
    REQUIRE(rejected);
    fixture.root.setFocus("data", id);
    loader.load(data);
    auto& local = fixture.root.addFolder("local");
    fixture.run_ctx.activateFocus(local.makeFocusScope());
    loader.load(data); // Inherits Root.
    local.setFocus("data", Lattice::InvalidObjectId);
    rejected = false;
    try { loader.load(data); } catch (const Lattice::Exception&) { rejected = true; }
    REQUIRE(rejected);
}
