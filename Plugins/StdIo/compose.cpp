#include <Lattice/Composition.hpp>
#include "IOSubsystem.hpp"
#include "YamlParser.hpp"
#include "TomlParser.hpp"
#include "TsvParser.hpp"

extern "C" void compose(NodeBuild root) {
    root.emplace<YamlParser, ParserAPI>("yaml");
    root.emplace<TomlParser, ParserAPI>("toml");
    root.emplace<TsvParser, ParserAPI>("tsv");
    root.emplaceShared<IOSubsystem>("io", root.children<ParserAPI>());
}
