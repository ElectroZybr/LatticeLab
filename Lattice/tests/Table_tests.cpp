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
    const NodeId node = fixture.run_ctx.nodes.builder.add(
        fixture.root,
        tableBlueprint,
        "results"
    );

    auto configure = fixture.run_ctx.nodes.configure(fixture.root);
    REQUIRE(configure.resolve<Table>(node) != nullptr);
    REQUIRE(configure.resolve<BasicTable>(node) != nullptr);
}

TEST(Table_BaseExportsViewAction, RuntimeFixture,
    "Базовый Table должен добавлять view каждой реализации.") {
    const NodeId node = fixture.run_ctx.nodes.builder.add(
        fixture.root,
        fixture.run_ctx.blueprints.id<Table>(),
        "results"
    );

    const ExportId view = fixture.run_ctx.nodes.exports.find(node, "view");
    REQUIRE(view != InvalidExportId);

    ActionContext context{node};
    fixture.run_ctx.nodes.exports.invoke(view, context);

    REQUIRE(context.output().size() == 1);
    const auto& output = std::get<ActionView>(context.output().front());
    REQUIRE(output.is<Table>());

    auto configure = fixture.run_ctx.nodes.configure(fixture.root);
    REQUIRE(&output.as<Table>() == configure.resolve<Table>(node));
}

TEST(Table_BaseAddsRows, RuntimeFixture,
    "Table должен добавлять строки через общий API и export.") {
    const NodeId node = fixture.run_ctx.nodes.builder.add(
        fixture.root,
        fixture.run_ctx.blueprints.id<Table>(),
        "results"
    );
    auto configure = fixture.run_ctx.nodes.configure(fixture.root);
    Table* table = configure.resolve<Table>(node);

    REQUIRE(table->addRows(2) == 0);
    REQUIRE(table->addRow() == 2);
    REQUIRE(table->rows() == 3);

    const ExportId addRow = fixture.run_ctx.nodes.exports.find(node, "addRow");
    REQUIRE(addRow != InvalidExportId);

    ActionContext context{node};
    fixture.run_ctx.nodes.exports.invoke(addRow, context);
    REQUIRE(table->rows() == 4);
    REQUIRE(std::get<Value>(context.output().front()).get<int64_t>() == 3);
}

TEST(Table_BaseAddsTypedRows, RuntimeFixture,
    "Table::addRow должен записывать типизированные значения без Value.") {
    BasicTable table;
    table.addColumn<std::string>("name");
    table.addColumn<double>("mass");
    table.addColumn<Particle>("particle");

    Table& generic = table;
    const size_t row = generic.addRow(
        std::string{"O"},
        15.999,
        Particle{8, -2.0}
    );

    REQUIRE(row == 0);
    REQUIRE(table.values<std::string>("name")[0] == "O");
    REQUIRE(table.values<double>("mass")[0] == 15.999);
    REQUIRE(table.values<Particle>("particle")[0] == (Particle{8, -2.0}));
}

}
