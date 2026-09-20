#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>

#include "ActionMap.hpp"
#include "InputAPI.hpp"
#include "KeybindLoader.hpp"
#include "LoaderAPI.hpp"
#include "TomlParser.hpp"

using Lattice::RuntimeFixture;

class TestInput final : public InputAPI {
public:
    std::string held;

    bool down(std::string_view trigger) const override {
        return trigger == held;
    }

    bool pressed(std::string_view) const override {
        return false;
    }

    bool released(std::string_view) const override {
        return false;
    }
};

struct KeybindsFixture : RuntimeFixture {
    TestInput* input = nullptr;
    ActionMap* map = nullptr;
    KeybindsLoader* loader = nullptr;

    KeybindsFixture() {
        blueprints.add<InputAPI>();
        blueprints.add<TestInput, InputAPI>();
        blueprints.add<ActionMap>();
        blueprints.add<LoaderAPI>();
        blueprints.add<KeybindsLoader, LoaderAPI>();

        root.add<TestInput>();
        root.add<ActionMap>();
        root.setFocus(Lattice::typeKey<ActionMap>(), root.find<ActionMap>().node->getId());
        root.add<KeybindsLoader>();
        root.configureBranch();

        input = root.require<TestInput>().getPtr();
        map = root.require<ActionMap>().getPtr();
        loader = root.require<KeybindsLoader>().getPtr();
    }

    void load(const Lattice::Table& keybinds) {
        Document document;
        document.root().emplace("keybinds", keybinds);
        const Lattice::Value* section = document.section(loader->section());
        if (section)
            loader->load(*section);
    }
};

TEST(Keybinds_SimpleAction, KeybindsFixture,
    "print = \"P\" должен биндить действие print на P.")
{
    int fires = 0;
    fixture.root.on("print", [&] { ++fires; });
    fixture.load(Lattice::Table{
        {"print", std::string("P")},
    });

    REQUIRE(fixture.map->bindCount() == 1);
    REQUIRE(fixture.map->hasBind("print", "P"));

    fixture.input->held = "P";
    fixture.map->tick();
    REQUIRE(fires == 1);
}

TEST(Keybinds_ArrayTriggerIgnoresExtraNumber, KeybindsFixture,
    "quit = [\"Ctrl+Q\", 12] должен биндить quit, лишнее число не ломает загрузку.")
{
    int fires = 0;
    fixture.root.on("quit", [&] { ++fires; });
    fixture.load(Lattice::Table{
        {"quit", Lattice::Array{std::string("Ctrl+Q"), int64_t{12}}},
    });

    REQUIRE(fixture.map->hasBind("quit", "Ctrl+Q"));
    REQUIRE(!fixture.map->hasBind("quit", "12"));

    fixture.input->held = "Ctrl+Q";
    fixture.map->tick();
    REQUIRE(fires == 1);
}

TEST(Keybinds_OpInKey, KeybindsFixture,
    "dt.add / dt.sub должны писать в слот dt, а не в dt.add.")
{
    double dt = 1.0;
    fixture.root.bind("dt", &dt);
    fixture.load(Lattice::Table{
        {"dt.add", Lattice::Array{std::string("]"), 0.5, std::string("hold")}},
        {"dt.sub", Lattice::Array{std::string("["), -0.5, std::string("hold")}},
    });

    REQUIRE(fixture.map->bindCount() == 2);
    REQUIRE(fixture.map->hasBind("dt", "]"));
    REQUIRE(fixture.map->hasBind("dt", "["));
    REQUIRE(!fixture.map->hasBind("dt.add", "]"));
    REQUIRE(!fixture.map->hasBind("dt.sub", "["));

    fixture.input->held = "]";
    fixture.map->tick();
    fixture.map->tick();
    REQUIRE(dt == 2.0);

    fixture.input->held = "[";
    fixture.map->tick();
    REQUIRE(dt == 1.5);
}

TEST(Keybinds_NestedTable, KeybindsFixture,
    "Вложенная таблица dt.add должна читаться так же, как dotted-ключ.")
{
    double dt = 0.0;
    fixture.root.bind("dt", &dt);
    fixture.load(Lattice::Table{
        {"dt", Lattice::Table{
            {"add", Lattice::Array{std::string("]"), 0.5}},
        }},
    });

    REQUIRE(fixture.map->hasBind("dt", "]"));

    fixture.input->held = "]";
    fixture.map->tick();
    REQUIRE(dt == 0.5);
}

TEST(Keybinds_ToggleInKey, KeybindsFixture,
    "flag.toggle должен переключать flag.")
{
    bool flag = false;
    fixture.root.bind("flag", &flag);
    fixture.load(Lattice::Table{
        {"flag.toggle", std::string("Space")},
    });

    REQUIRE(fixture.map->hasBind("flag", "Space"));

    fixture.input->held = "Space";
    fixture.map->tick();
    REQUIRE(flag);
}

TEST(Keybinds_ReloadDoesNotDuplicate, KeybindsFixture,
    "Повторная загрузка той же таблицы не должна плодить бинды.")
{
    const Lattice::Table keybinds{
        {"print", std::string("P")},
        {"dt.add", Lattice::Array{std::string("]"), 0.001, std::string("hold")}},
        {"dt.sub", Lattice::Array{std::string("["), -0.001, std::string("hold")}},
    };

    fixture.load(keybinds);
    fixture.load(keybinds);

    REQUIRE(fixture.map->bindCount() == 3);
}

TEST(Keybinds_FileSignature, KeybindsFixture,
    "Config/keybinds.toml должен грузиться в слоты print и dt.")
{
    double dt = 1.0;
    int prints = 0;
    fixture.root.bind("dt", &dt);
    fixture.root.on("print", [&] { ++prints; });
    TomlParser parser;
    const Document doc = parser.parseFile("Config/keybinds.toml");
    const Lattice::Value* section = doc.section(fixture.loader->section());
        if (section)
            fixture.loader->load(*section);

    REQUIRE(fixture.map->hasBind("print", "P"));
    REQUIRE(fixture.map->hasBind("quit", "Ctrl+Q"));
    REQUIRE(fixture.map->hasBind("dt", "]"));
    REQUIRE(fixture.map->hasBind("dt", "["));

    fixture.input->held = "P";
    fixture.map->tick();
    REQUIRE(prints == 1);

    fixture.input->held = "]";
    fixture.map->tick();
    REQUIRE(dt == 1.001);
}
