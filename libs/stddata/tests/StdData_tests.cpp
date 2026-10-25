// Plain data structures: no NodeSystem, builders or plugin loading in this executable.
#include <iostream>
#include <stdexcept>
#include <Lattice/kernel/TableAPI.hpp>
#include <stddata/CSR.hpp>
#include <stddata/NamedSoA.hpp>
#include <stddata/SoA.hpp>

namespace {
struct Mass { using type = float; };
struct Charge { using type = int; };

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F&& fn) {
    bool failed = false;
    try { fn(); } catch (const Lattice::ExceptionBase&) { failed = true; }
    check(failed, "Expected a Lattice exception");
}

void soaColumnsByName() {
    StdData::SoA soa;
    soa.addCol<Mass>();
    soa.addCol<Charge>();
    soa.resize(3);
    auto mass = soa.span<float>(Lattice::typeName<Mass>());
    mass[2] = 9.0f;
    check(soa.at<Mass>(2) == 9.0f && mass.size() == 3, "named span writes the column");
    bool wrongType = false, missing = false;
    try { (void)soa.span<int>(Lattice::typeName<Mass>()); } catch (const Lattice::ExceptionBase&) { wrongType = true; }
    try { (void)soa.span<float>("nope"); } catch (const Lattice::ExceptionBase& e) {
        missing = std::string(e.what()).find(Lattice::typeName<Charge>()) != std::string::npos;
    }
    check(wrongType && missing, "wrong type and missing name are reported, with the column list");
}

void soaImplementsTableContract() {
    StdData::SoA soa;
    soa.addCol<Mass>();
    soa.addCol<Charge>();
    soa.resize(2);
    soa.at<Mass>(0) = 1.008f;
    soa.at<Mass>(1) = 4.003f;
    soa.at<Charge>(0) = 1;
    soa.at<Charge>(1) = 0;

    const Lattice::Table& table = soa;
    check(table.rows() == 2, "rows");
    check(table.columns() == 2, "columns");
    check(table.column("Mass").type().is<float>(), "column type");
    check(table.column("Mass").values<float>()[1] == 4.003f, "column values");
    check(table.column("Charge").values<int>()[0] == 1, "second column");

    soa.removeCol<Mass>();
    check(table.columns() == 1, "column removed");
    check(table.column(0).name() == "Charge", "remaining column");
    rejects([&] { soa.removeCol<Mass>(); });
}

void soaAddsTypedRowsAndKeepsDataOnGrowth() {
    StdData::SoA soa;
    soa.addCol<Mass>();
    soa.addCol<Charge>();

    Lattice::Table& table = soa;
    check(table.addRow(1.008f, int{1}) == 0, "first row");
    for (int i = 1; i < 100; ++i)
        table.addRow(float(i), i);
    check(soa.at<Mass>(0) == 1.008f && soa.at<Charge>(0) == 1, "data survives relayout");
    check(soa.at<Charge>(99) == 99, "last row");
    soa.swapRows(0, 99);
    check(soa.at<Charge>(0) == 99 && soa.at<Mass>(99) == 1.008f, "swap rows");
    rejects([&] { table.addRow(1.0f); });
}

void namedSoAFindsRowsByName() {
    StdData::NamedSoA soa;
    soa.addCol<Mass>();
    const size_t h = soa.addRow("H", 1.008f);
    const size_t he = soa.addRow("He", 4.003f);
    check(soa.requireRow("He") == he && soa.find("H") == h, "named rows");
    check(soa.hasRow("H") && !soa.hasRow("Li"), "has row");
    rejects([&] { (void)soa.requireRow("Li"); });
}

void csrGroupsValues() {
    StdData::CSR<uint32_t> csr;
    csr.resize(3, 3);
    csr.offsets()[0] = 0;
    csr.offsets()[1] = 1;
    csr.offsets()[2] = 3;
    csr.values()[0] = 7;
    csr.values()[1] = 8;
    csr.values()[2] = 9;
    check(csr.size() == 2 && csr[0].size() == 1 && csr[1].size() == 2 && csr[1][1] == 9, "csr rows");
}
}

int main() {
    soaImplementsTableContract();
    soaColumnsByName();
    soaAddsTypedRowsAndKeepsDataOnGrowth();
    namedSoAFindsRowsByName();
    csrGroupsValues();
    std::cout << "StdData: table contract, named rows and CSR passed\n";
}
