#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Lattice.hpp>


namespace Lattice {

struct BootChild {
    bool configured = false;

    void configure(Node&) {
        configured = true;
    }
};

struct BootHost {
    BootChild* child = nullptr;
    bool loaded = false;

    explicit BootHost(Node& branch) {
        branch.add<BootChild>();
    }

    void configure(Node& branch) {
        child = branch.require<BootChild>().getPtr();
        REQUIRE(child);
        REQUIRE(!child->configured);

        branch.on("load", [this] {
            REQUIRE(child->configured);
            loaded = true;
        });
    }
};

TEST(Runtime_LoadAfterConfigure, RuntimeFixture,
    "Стартовый load должен вызываться после configureBranch, когда дети уже сконфигурированы.")
{
    fixture.blueprints.add<BootChild>();
    fixture.blueprints.add<BootHost>();
    fixture.root.add<BootHost>();

    auto host = fixture.root.require<BootHost>();
    REQUIRE(host->child == nullptr);
    REQUIRE(!host->loaded);

    fixture.root.configureBranch();

    REQUIRE(host->child);
    REQUIRE(host->child->configured);
    REQUIRE(!host->loaded);

    const ObjectId load = fixture.run_ctx.find("load");
    REQUIRE(load != InvalidObjectId);

    fixture.run_ctx.bindings.invoke(load);

    REQUIRE(host->loaded);
}

TEST(Runtime_LoadMissingIsSafe, RuntimeFixture,
    "Если действия load нет, стартовая загрузка должна просто пропускаться.")
{
    REQUIRE(fixture.run_ctx.find("load") == InvalidObjectId);
    fixture.run_ctx.bindings.invoke(fixture.run_ctx.find("load"));
}

} // namespace Lattice
