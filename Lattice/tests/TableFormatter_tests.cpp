#include <format>
#include <string>
#include <vector>

#include <Lattice/Lattice.hpp>
#include <Lattice/tests/RuntimeFixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

std::vector<std::string> plainLines(const TableFormatter::View& view) {
    std::vector<std::string> result;
    for (const TextFormatter line : view)
        result.push_back(line.plain());
    return result;
}

}

TEST(TableFormatter_RendersAsciiTable, RuntimeFixture,
    "TableFormatter::View должен рисовать выровненную ASCII-таблицу.") {
    BasicTable table;
    table.resize(2);
    table.addColumn<std::string>("name");
    table.addColumn<double>("mass");
    table.values<std::string>("name")[0] = "H";
    table.values<std::string>("name")[1] = "He";
    table.values<double>("mass")[0] = 1.5;
    table.values<double>("mass")[1] = 4.0;

    TableFormatter renderer;
    TableFormatter::Desc desc;
    desc.style.borders = TableFormatter::Borders::Ascii;
    const auto lines = plainLines(renderer.view(table, desc));

    REQUIRE(lines.size() == 6);
    REQUIRE(lines[0] == "+------+------+");
    REQUIRE(lines[1] == "| name | mass |");
    REQUIRE(lines[2] == "+------+------+");
    REQUIRE(lines[3] == "| H    |  1.5 |");
    REQUIRE(lines[4] == "| He   |    4 |");
    REQUIRE(lines[5] == "+------+------+");
}

TEST(TableFormatter_CustomFormatterAndOpenBorders, RuntimeFixture,
    "Renderer должен поддерживать custom типы и таблицу без внешней рамки.") {
    struct Pair { int left = 0; int right = 0; };

    BasicTable table;
    table.resize(1);
    table.addColumn<Pair>("pair");
    table.values<Pair>("pair")[0] = {2, 7};

    TableFormatter renderer;
    renderer.formats().add<Pair>([](const Pair& value) {
        return std::format("{}:{}", value.left, value.right);
    });

    TableFormatter::Desc desc;
    desc.style.rules = TableFormatter::Rules::Header;
    const auto lines = plainLines(renderer.view(table, desc));

    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == " pair ");
    REQUIRE(lines[1] == "──────");
    REQUIRE(lines[2] == " 2:7  ");
}

TEST(TableFormatter_ReportsTruncatedRows, RuntimeFixture,
    "Formatter должен сообщать, сколько строк не выведено.") {
    BasicTable table;
    table.addColumn<int>("value");
    table.addRow(1);
    table.addRow(2);
    table.addRow(3);

    TableFormatter formatter;
    TableFormatter::Desc desc;
    desc.maxRows = 1;
    const auto lines = plainLines(formatter.view(table, desc));

    REQUIRE(lines.back() == "... 2 more rows");
}

TEST(TableFormatter_UnlimitedRows, RuntimeFixture,
    "Desc::Unlimited должен отключать ограничение строк.") {
    BasicTable table;
    table.addColumn<int>("value");
    for (int value = 0; value < 40; ++value)
        table.addRow(value);

    TableFormatter formatter;
    TableFormatter::Desc desc;
    desc.maxRows = TableFormatter::Desc::Unlimited;
    const auto lines = plainLines(formatter.view(table, desc));

    REQUIRE(lines.size() == 44);
    REQUIRE(lines.back() != "... 8 more rows");
}

}
