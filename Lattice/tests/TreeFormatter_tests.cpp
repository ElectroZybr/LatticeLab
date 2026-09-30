#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Tools/TreeFormatter.hpp>

namespace Lattice {

TEST(TreeFormatter_CustomAsciiStyle, Fixture,
    "TreeFormatter должен форматировать дерево без привязки к Logger.") {
    TreeFormatStyle style;
    style.glyphs = TreeStyles::Ascii;

    TreeFormatter tree("Root", style);
    auto& branch = tree.branch("first");
    branch.node("child");
    tree.node("last");

    REQUIRE(tree.format().plain() ==
        "Root\n"
        "+- first\n"
        "|  \\- child\n"
        "\\- last"
    );
}

TEST(TreeFormatter_ComposesStylesLinearly, Fixture,
    "Стиль строки действует как внешняя обёртка для вложенного форматирования.") {
    TreeFormatter tree("Context");
    tree.node("<wrn>λ</> action");

    const std::string markup = tree.format().markup();
    REQUIRE(markup.find("<#ffff55>λ</>") != std::string::npos);
    REQUIRE(markup.find(" action") != std::string::npos);
}

}
