#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>


namespace Lattice {

struct ContextDummy {};

TEST(Context_GetSlotStable, RuntimeFixture,
    "getSlot с одним именем должен возвращать один и тот же слот.")
{
    const SlotId a = fixture.run_ctx.getSlot("print");
    const SlotId b = fixture.run_ctx.getSlot("print");
    const SlotId c = fixture.run_ctx.getSlot("load");

    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST(Context_FindSlotDoesNotCreate, RuntimeFixture,
    "findSlot и active не должны создавать слот.")
{
    REQUIRE(fixture.run_ctx.findSlot("print") == InvalidSlotId);
    REQUIRE(!Objects::valid(fixture.run_ctx.active("print")));
    REQUIRE(fixture.run_ctx.findSlot("print") == InvalidSlotId);
}

TEST(Context_GetSlotCreatesInactive, RuntimeFixture,
    "getSlot создаёт неактивный слот.")
{
    const SlotId slot = fixture.run_ctx.getSlot("print");

    REQUIRE(slot != InvalidSlotId);
    REQUIRE(fixture.run_ctx.findSlot("print") == slot);
    REQUIRE(!Objects::valid(fixture.run_ctx.get(slot)));
    REQUIRE(!Objects::valid(fixture.run_ctx.active("print")));
}

TEST(Context_Activate, RuntimeFixture,
    "activate должен привязать объект к слоту.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>("first");

    const auto dummy = fixture.root.find<ContextDummy>("first");
    REQUIRE(dummy.node);

    const ObjectId id = dummy.node->getId();
    const SlotId slot = fixture.run_ctx.getSlot("focus");

    fixture.run_ctx.activate(slot, id);

    REQUIRE(fixture.run_ctx.get(slot) == id);
    REQUIRE(fixture.run_ctx.active("focus") == id);
}

TEST(Context_ActivateOverwrite, RuntimeFixture,
    "activate должен заменять текущий объект слота.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>("first");
    fixture.root.add<ContextDummy>("second");

    const auto first = fixture.root.find<ContextDummy>("first");
    const auto second = fixture.root.find<ContextDummy>("second");
    REQUIRE(first.node);
    REQUIRE(second.node);

    const SlotId slot = fixture.run_ctx.findSlot("ContextDummy");
    REQUIRE(slot != InvalidSlotId);
    REQUIRE(fixture.run_ctx.get(slot) == first.node->getId());

    fixture.run_ctx.activate(slot, second.node->getId());

    REQUIRE(fixture.run_ctx.get(slot) == second.node->getId());
    REQUIRE(fixture.run_ctx.active("ContextDummy") == second.node->getId());
}

TEST(Context_GetInvalid, RuntimeFixture,
    "get по несуществующему слоту должен возвращать InvalidObjectId.")
{
    REQUIRE(!Objects::valid(fixture.run_ctx.get(InvalidSlotId)));
    REQUIRE(!Objects::valid(fixture.run_ctx.get(99)));
}

TEST(Context_ActivateUnknownObject, RuntimeFixture,
    "activate неизвестного объекта должен бросать исключение.")
{
    const SlotId slot = fixture.run_ctx.getSlot("print");
    bool thrown = false;

    try {
        fixture.run_ctx.activate(slot, 999);
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Context_ActivateUnknownSlot, RuntimeFixture,
    "activate неизвестного слота не должен менять контекст.")
{
    fixture.blueprints.blueprint<ContextDummy>();
    fixture.root.add<ContextDummy>();

    const auto dummy = fixture.root.find<ContextDummy>();
    REQUIRE(dummy.node);

    fixture.run_ctx.activate(99, dummy.node->getId());

    REQUIRE(!Objects::valid(fixture.run_ctx.get(99)));
}

TEST(Context_Clear, RuntimeFixture,
    "clear должен удалить слоты.")
{
    fixture.run_ctx.getSlot("print");
    REQUIRE(fixture.run_ctx.findSlot("print") != InvalidSlotId);

    fixture.run_ctx.clear();

    REQUIRE(fixture.run_ctx.findSlot("print") == InvalidSlotId);
    REQUIRE(!Objects::valid(fixture.run_ctx.active("print")));
}

} // namespace Lattice
