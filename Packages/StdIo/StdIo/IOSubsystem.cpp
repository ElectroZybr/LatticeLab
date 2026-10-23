#include "IOSubsystem.hpp"
#include <utility>
#include <Lattice/Tools/Exception.hpp>

namespace {
using Error = Lattice::Exception<IOSubsystem>;
}

IOSubsystem::IOSubsystem(std::vector<ParserAPI*> parsers) : parsers_(std::move(parsers)) {
    for (auto* parser : parsers_)
        if (!parser) throw Error("IO requires non-null parsers");
}

Document IOSubsystem::read(const std::filesystem::path& path) const {
    return requireParser(path).parseFile(path);
}

void IOSubsystem::write(const std::filesystem::path& path, const Document& document) const {
    requireParser(path).writeFile(path, document);
}

ParserAPI& IOSubsystem::requireParser(const std::filesystem::path& path) const {
    ParserAPI* selected = nullptr;
    const auto extension = path.extension().string();
    for (auto* parser : parsers_) {
        if (!parser->supportsExtension(extension)) continue;
        if (selected) throw Error("Multiple parsers support '{}'", extension);
        selected = parser;
    }
    if (!selected) throw Error("Parser for '{}' not found", extension);
    return *selected;
}
