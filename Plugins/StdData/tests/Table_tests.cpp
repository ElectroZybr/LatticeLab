#include <Lattice/Kernel/Table.hpp>
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

}
