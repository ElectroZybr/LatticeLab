#include <Lattice/Kernel/Registry.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include <limits>
#include <string>

namespace Lattice {

struct RegistryDummy {
    std::string name;
    bool exists = true;
    void* node = nullptr;
};

using TestRegistry = Registry<RegistryDummy, uint32_t, std::string>;

TEST(Registry_Create, RuntimeFixture,
    "create должен добавлять объект и возвращать его id.")
{
    TestRegistry registry;

    const uint32_t a = registry.create({"first"});
    const uint32_t b = registry.create({"second"});

    REQUIRE(a != b);
    REQUIRE(registry.size() == 2);
    REQUIRE(registry.get(a));
    REQUIRE(registry.get(b));
}

TEST(Registry_Get, RuntimeFixture,
    "get должен получать объект напрямую по id.")
{
    TestRegistry registry;

    const uint32_t id = registry.create({"first"});
    const RegistryDummy* object = registry.get(id);

    REQUIRE(object);
    REQUIRE(object->name == "first");
}

TEST(Registry_Find, RuntimeFixture,
    "find должен находить объект по имени.")
{
    TestRegistry registry;

    const uint32_t id = registry.create({"first"});

    REQUIRE(registry.find("first") == id);
    REQUIRE(registry.has("first"));
    REQUIRE(!registry.has("unknown"));
}

TEST(Registry_GetUnknown, RuntimeFixture,
    "get неизвестного id должен возвращать nullptr.")
{
    TestRegistry registry;

    REQUIRE(registry.get(99) == nullptr);
    REQUIRE(registry.get(std::numeric_limits<uint32_t>::max()) == nullptr);
}

TEST(Registry_RequireUnknown, RuntimeFixture,
    "require неизвестного id должен бросать исключение.")
{
    TestRegistry registry;

    bool thrown = false;

    try {
        registry.require(99);
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
}

TEST(Registry_Alias, RuntimeFixture,
    "alias должен добавлять дополнительное имя объекта.")
{
    TestRegistry registry;

    const uint32_t id = registry.create({"first"});
    registry.alias(id, "alias");

    REQUIRE(registry.find("first") == id);
    REQUIRE(registry.find("alias") == id);
}

TEST(Registry_Destroy, RuntimeFixture,
    "destroy должен освобождать слот и удалять его имена.")
{
    TestRegistry registry;

    const uint32_t id = registry.create({"first"});
    registry.alias(id, "alias");

    registry.destroy(id);

    REQUIRE(registry.get(id) == nullptr);
    REQUIRE(!registry.has("first"));
    REQUIRE(!registry.has("alias"));
}

TEST(Registry_ReusesId, RuntimeFixture,
    "create должен переиспользовать освобождённый id.")
{
    TestRegistry registry;

    const uint32_t first = registry.create({"first"});
    registry.create({"second"});

    registry.destroy(first);

    const uint32_t reused = registry.create({"third"});

    REQUIRE(reused == first);
    REQUIRE(registry.get(reused));
    REQUIRE(registry.get(reused)->name == "third");
}

TEST(Registry_Clear, RuntimeFixture,
    "clear должен удалить все объекты и имена.")
{
    TestRegistry registry;

    registry.create({"first"});
    registry.create({"second"});

    registry.clear();

    REQUIRE(registry.size() == 0);
    REQUIRE(!registry.has("first"));
    REQUIRE(!registry.has("second"));
}

TEST(Registry_CreateDuplicate, RuntimeFixture,
    "create дублирующегося имени должен бросать исключение.")
{
    TestRegistry registry;

    registry.create({"first"});

    bool thrown = false;

    try {
        registry.create({"first"});
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(registry.size() == 1);
    REQUIRE(registry.find("first") == 0);
}

TEST(Registry_AliasDuplicate, RuntimeFixture,
    "alias существующего имени должен бросать исключение.")
{
    TestRegistry registry;

    const auto a = registry.create({"first"});
    const auto b = registry.create({"second"});

    registry.alias(a, "alias");

    bool thrown = false;

    try {
        registry.alias(b, "alias");
    } catch (const Exception&) {
        thrown = true;
    }

    REQUIRE(thrown);
    REQUIRE(registry.find("alias") == a);
}

} // namespace Lattice