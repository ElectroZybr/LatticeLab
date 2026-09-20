#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>
#include "Lattice/Kernel/Objects.hpp"

#include "ActionMap.hpp"
#include "InputAPI.hpp"

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

struct ActionMapFixture : RuntimeFixture {
    TestInput* input = nullptr;
    ActionMap* map = nullptr;

    ActionMapFixture() {
        blueprints.add<InputAPI>();
        blueprints.add<TestInput, InputAPI>();
        blueprints.add<ActionMap>();

        root.add<TestInput>();
        root.add<ActionMap>();
        root.configureBranch();

        input = root.require<TestInput>().getPtr();
        map = root.require<ActionMap>().getPtr();
    }
};

TEST(ActionMap_BindIsIdempotent, ActionMapFixture,
    "Повторная загрузка одинакового бинда не должна дублировать его.")
{
    fixture.map->bind("print", "P");
    fixture.map->bind("print", "P");
    fixture.map->bind("print", "P");

    REQUIRE(fixture.map->bindCount() == 1);
    REQUIRE(fixture.map->hasBind("print", "P"));
}

TEST(ActionMap_DifferentTriggers, ActionMapFixture,
    "Один глагол может иметь несколько разных триггеров.")
{
    fixture.map->bind("print", "P");
    fixture.map->bind("print", "MouseLeft");

    REQUIRE(fixture.map->bindCount() == 2);
    REQUIRE(fixture.map->hasBind("print", "P"));
    REQUIRE(fixture.map->hasBind("print", "MouseLeft"));
}

TEST(ActionMap_ReloadDoesNotDoubleFire, ActionMapFixture,
    "После повторного bind действие должно сработать один раз на нажатие.")
{
    int fires = 0;
    fixture.root.on("print", [&] { ++fires; });
    fixture.map->bind("print", "P");
    fixture.map->bind("print", "P");

    fixture.input->held = "P";
    fixture.map->tick();
    fixture.map->tick();

    REQUIRE(fixture.map->bindCount() == 1);
    REQUIRE(fires == 1);
}

TEST(ActionMap_ReboundMode, ActionMapFixture,
    "Повторный bind с другим режимом должен заменить старый, а не добавить второй.")
{
    int fires = 0;
    fixture.root.on("print", [&] { ++fires; });
    fixture.map->bind("print", "P");
    fixture.map->bind("print", "P", ActionMode::OnHold);

    REQUIRE(fixture.map->bindCount() == 1);

    fixture.input->held = "P";
    fixture.map->tick();
    fixture.map->tick();

    REQUIRE(fires == 2);
}

TEST(ActionMap_Toggle, ActionMapFixture,
    "bindToggle должен переключать bool-биндинг.")
{
    bool flag = false;
    fixture.root.bind("flag", &flag);
    fixture.map->bindToggle("flag", "T");
    fixture.map->bindToggle("flag", "T");

    REQUIRE(fixture.map->bindCount() == 1);

    fixture.input->held = "T";
    fixture.map->tick();
    REQUIRE(flag);

    fixture.input->held.clear();
    fixture.map->tick();
    fixture.input->held = "T";
    fixture.map->tick();
    REQUIRE(!flag);
}

TEST(ActionMap_Add, ActionMapFixture,
    "bindAdd должен прибавлять delta к числу.")
{
    double dt = 1.0;
    fixture.root.bind("dt", &dt);
    fixture.map->bindAdd("dt", "]", 0.5);
    fixture.map->bindAdd("dt", "]", 0.5);

    REQUIRE(fixture.map->bindCount() == 1);

    fixture.input->held = "]";
    fixture.map->tick();

    REQUIRE(dt == 1.5);
}

TEST(ActionMap_PressHoldRelease, ActionMapFixture,
    "tick должен различать press, hold и release.")
{
    fixture.root.on("print", [] {});
    fixture.map->bind("print", "P");

    const auto slot = fixture.run_ctx.roles.find("print");
    REQUIRE(slot != Lattice::InvalidRoleId);

    fixture.input->held = "P";
    fixture.map->tick();
    REQUIRE(fixture.map->pressed(slot));
    REQUIRE(fixture.map->down(slot));
    REQUIRE(!fixture.map->released(slot));

    fixture.map->tick();
    REQUIRE(!fixture.map->pressed(slot));
    REQUIRE(fixture.map->down(slot));
    REQUIRE(!fixture.map->released(slot));

    fixture.input->held.clear();
    fixture.map->tick();
    REQUIRE(!fixture.map->pressed(slot));
    REQUIRE(!fixture.map->down(slot));
    REQUIRE(fixture.map->released(slot));
}

TEST(ActionMap_ClearBinds, ActionMapFixture,
    "clearBinds должен удалить все бинды.")
{
    fixture.map->bind("print", "P");
    fixture.map->bind("load", "Ctrl+O");
    REQUIRE(fixture.map->bindCount() == 2);

    fixture.map->clearBinds();

    REQUIRE(fixture.map->bindCount() == 0);
    REQUIRE(!fixture.map->hasBind("print", "P"));
}

TEST(ActionMap_HasBindMissing, ActionMapFixture,
    "hasBind не должен создавать слот для неизвестного глагола.")
{
    REQUIRE(!fixture.map->hasBind("missing", "P"));
    REQUIRE(fixture.run_ctx.resolveFocus(Lattice::InvalidFocusScopeId, fixture.run_ctx.roles.find("missing")) == Lattice::InvalidObjectId);
}

TEST(ActionMap_FocusChainSwitch, ActionMapFixture) {
    auto& ctx = fixture.run_ctx;
    int rootCalls = 0, localCalls = 0;
    fixture.root.on("move", [&] { ++rootCalls; });
    auto& left = fixture.root.addFolder("left");
    auto& right = fixture.root.addFolder("right");
    const auto leftScope = left.makeFocusScope();
    const auto rightScope = right.makeFocusScope();
    left.on("move", [&] { ++localCalls; });
    fixture.map->bind("move", "M", ActionMode::OnHold);
    const auto role = ctx.roles.find("move");
    fixture.input->held = "M";
    ctx.activateFocus(leftScope);
    fixture.map->tick();
    REQUIRE(localCalls == 1);
    REQUIRE(rootCalls == 0);
    ctx.activateFocus(rightScope);
    fixture.map->tick();
    REQUIRE(rootCalls == 1);
    right.setFocus("move", Lattice::InvalidObjectId);
    fixture.map->tick();
    REQUIRE(rootCalls == 1);
    ctx.resetFocus(rightScope, role);
    fixture.map->tick();
    REQUIRE(rootCalls == 2);
    REQUIRE(ctx.roles.find("move") == role);
    REQUIRE(fixture.map->hasBind("move", "M"));
}
