#include <Lattice/Kernel/TableAPI.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

#include "SoA.hpp"

namespace StdData {
namespace {

struct Mass { using type = float; };
struct Charge { using type = int; };

}

TEST(SoA_ImplementsTableContract, Lattice::TestFixture,
    "SoA должна отдавать типизированные колонки через общий Table API.") {
    SoA soa;
    soa.addCol<Mass>();
    soa.addCol<Charge>();
    soa.resize(2);
    soa.at<Mass>(0) = 1.008f;
    soa.at<Mass>(1) = 4.003f;
    soa.at<Charge>(0) = 1;
    soa.at<Charge>(1) = 0;

    const Lattice::Table& table = soa;
    REQUIRE(table.rows() == 2);
    REQUIRE(table.columns() == 2);
    REQUIRE(table.column("Mass").type().is<float>());
    REQUIRE(table.column("Mass").values<float>()[1] == 4.003f);
    REQUIRE(table.column("Charge").values<int>()[0] == 1);

    soa.removeCol<Mass>();
    REQUIRE(table.columns() == 1);
    REQUIRE(table.column(0).name() == "Charge");
}

TEST(SoA_InheritsTableViewAction, Lattice::RuntimeFixture,
    "SoA должна получать базовые exports Table.") {
    Lattice::BlueprintRegister::add<SoA, Lattice::Table>(fixture.run_ctx.blueprints);
    const Lattice::NodeId node = fixture.run_ctx.nodes.builder.build(
        fixture.root,
        fixture.run_ctx.blueprints.id<SoA>(),
        "data"
    );

    const Lattice::ExportId view = fixture.run_ctx.nodes.exports.find(node, "view");
    REQUIRE(view != Lattice::InvalidExportId);

    Lattice::ActionContext context{node};
    fixture.run_ctx.nodes.exports.invoke(view, context);
    REQUIRE(std::get<Lattice::ActionView>(context.output().front()).is<Lattice::Table>());
}

TEST(SoA_AddsTypedRowsThroughTable, Lattice::TestFixture,
    "SoA должна поддерживать общий типизированный addRow.") {
    SoA soa;
    soa.addCol<Mass>();
    soa.addCol<Charge>();

    Lattice::Table& table = soa;
    REQUIRE(table.addRow(1.008f, int{1}) == 0);
    REQUIRE(soa.at<Mass>(0) == 1.008f);
    REQUIRE(soa.at<Charge>(0) == 1);
}

}
