#include <string>

#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

struct Particle {
    int species = 0;
    double charge = 0.0;

    friend bool operator==(const Particle&, const Particle&) = default;
};

}

TEST(Table_BasicTableStoresTypedColumns, RuntimeFixture,
    "BasicTable должна хранить типизированные колонки без преобразования в Value.") {
    BasicTable table;
    table.resize(2);
    table.addColumn<std::string>("name");
    table.addColumn<double>("mass");

    table.values<std::string>("name")[0] = "H";
    table.values<std::string>("name")[1] = "He";
    table.values<double>("mass")[0] = 1.008;
    table.values<double>("mass")[1] = 4.003;

    const Table& view = table;
    REQUIRE(view.rows() == 2);
    REQUIRE(view.columns() == 2);
    REQUIRE(view.column("name").type().is<std::string>());
    REQUIRE(view.column("name").values<std::string>()[1] == "He");
    REQUIRE(view.column("mass").values<double>()[0] == 1.008);
}

TEST(Table_BasicTableAcceptsCustomTypes, RuntimeFixture,
    "Таблица ядра не должна ограничивать данные закрытым набором типов.") {
    BasicTable table;
    table.resize(1);
    table.addColumn<Particle>("particle");
    table.values<Particle>("particle")[0] = {.species = 8, .charge = -2.0};

    const auto column = static_cast<const Table&>(table).column(0);
    REQUIRE(column.type().is<Particle>());
    REQUIRE(column.values<Particle>()[0] == (Particle{8, -2.0}));
}

TEST(Table_DefaultImplementationBuildsAsNode, RuntimeFixture,
    "Стандартная таблица должна строиться через обычный Table blueprint.") {
    const BlueprintId tableBlueprint = fixture.run_ctx.blueprints.id<Table>();
    const NodeId node = fixture.run_ctx.nodes.builder.build(
        fixture.root,
        tableBlueprint,
        "results"
    );

    auto configure = fixture.run_ctx.nodes.configure(fixture.root);
    REQUIRE(configure.resolve<Table>(node) != nullptr);
    REQUIRE(configure.resolve<BasicTable>(node) != nullptr);
}

}
