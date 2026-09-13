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

Value makeSoAData(std::string target) {
    return Table{
        {"target", std::move(target)},
        {"columns", Array{std::string("Mass"), std::string("Valence")}},
        {"rows", Array{
            Array{Value{1.008}, Value{int64_t{1}}},
            Array{Value{4.003}, Value{int64_t{0}}},
        }},
    };
}

} // namespace

TEST(SoALoader_LoadIntoWrapper, RuntimeFixture,
    "Лоадер должен найти SoA под активной обёрткой в контексте и записать строки.")
{
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.blueprints.blueprint<Wrapper>();
    fixture.root.add<Wrapper>();

    SoALoader loader;
    loader.configure(fixture.root);

    const Value data = makeSoAData("Wrapper");
    loader.load(&data);

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
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.blueprints.blueprint<Wrapper>();
    fixture.root.add<Wrapper>();

    SoALoader loader;
    loader.configure(fixture.root);

    const Value data = Table{
        {"target", std::string("Wrapper")},
        {"columns", Array{std::string("Name"), std::string("Mass"), std::string("Valence")}},
        {"rows", Array{
            Array{std::string("H"), Value{1.008}, Value{int64_t{1}}},
            Array{std::string("He"), Value{4.003}, Value{int64_t{0}}},
        }},
    };
    loader.load(&data);

    auto soa = fixture.root.require<Wrapper>("default")->soa;
    REQUIRE(soa->size() == 2);
    REQUIRE(std::strcmp(soa->at<Name>(0).data(), "H") == 0);
    REQUIRE(std::strcmp(soa->at<Name>(1).data(), "He") == 0);
}

TEST(SoALoader_MissingTarget, RuntimeFixture,
    "Лоадер должен падать, если цель не активна в контексте.")
{
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.blueprints.blueprint<Wrapper>();
    fixture.root.add<Wrapper>();

    SoALoader loader;
    loader.configure(fixture.root);

    bool thrown = false;
    try {
        const Value data = makeSoAData("AtomData");
        loader.load(&data);
    } catch (const Lattice::Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(SoALoader_MissingColumn, RuntimeFixture,
    "Лоадер должен падать, если колонки нет в буфере.")
{
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.blueprints.blueprint<Wrapper>();
    fixture.root.add<Wrapper>();

    SoALoader loader;
    loader.configure(fixture.root);

    const Value data = Table{
        {"target", std::string("Wrapper")},
        {"columns", Array{std::string("Charge")}},
        {"rows", Array{Array{Value{1.0}}}},
    };

    bool thrown = false;
    try {
        loader.load(&data);
    } catch (const Lattice::Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(SoALoader_InspectEmpty, RuntimeFixture,
    "inspect пустого SoA не должен падать.")
{
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.root.add<StdData::SoA>();

    auto soa = fixture.root.require<StdData::SoA>();
    REQUIRE(soa->size() == 0);
    soa->inspect("empty");
}

TEST(SoALoader_LoadAtomDataFile, RuntimeFixture,
    "Config/atomData.toml должен заполнить SoA у AtomData.")
{
    fixture.blueprints.blueprint<StdData::SoA>();
    fixture.blueprints.blueprint<AtomData>();
    fixture.root.add<AtomData>();

    TomlParser parser;
    const Document doc = parser.parseFile("Config/atomData.toml");
    const Value* data = doc.get("SoAData");
    REQUIRE(data);
    REQUIRE(data->is<Table>());

    const auto& table = std::get<Table>(*data);
    const auto rowsIt = table.find("rows");
    REQUIRE(rowsIt != table.end());
    REQUIRE(rowsIt->second.is<Array>());
    REQUIRE(std::get<Array>(rowsIt->second)[0].is<Array>());

    SoALoader loader;
    loader.configure(fixture.root);
    loader.load(data);

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
