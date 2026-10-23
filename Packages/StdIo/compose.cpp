#include <Lattice/Composition.hpp>
#include <StdIo/IOSubsystem.hpp>
#include <StdIo/YamlParser.hpp>
#include <StdIo/TomlParser.hpp>
#include <StdIo/TsvParser.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "StdIo",
    .name = "Standard input/output",
    .version = {1, 0, 0},
};

extern "C" void compose(NodeBuild root) {
    root.emplace<YamlParser, ParserAPI>("yaml");
    root.emplace<TomlParser, ParserAPI>("toml");
    root.emplace<TsvParser, ParserAPI>("tsv");
    root.emplaceShared<IOSubsystem>("io", root.children<ParserAPI>());
}
