#include "IOSubsystem.hpp"
#include <mutex>
#include <utility>
#include <Lattice/tools/Exception.hpp>

namespace {
using Error = Lattice::Exception<IOSubsystem>;
}

IOSubsystem::IOSubsystem(std::vector<ParserAPI*> parsers) {
    setParsers(std::move(parsers));
}

void IOSubsystem::setParsers(std::vector<ParserAPI*> parsers) {
    for (auto* parser : parsers)
        if (!parser) throw Error("IO requires non-null parsers");
    std::unique_lock lock(mutex_);
    parsers_ = std::move(parsers);
}

Document IOSubsystem::read(const std::filesystem::path& path) const {
    std::shared_lock lock(mutex_);
    return requireParser(path).parseFile(path);
}

void IOSubsystem::write(const std::filesystem::path& path, const Document& document) const {
    std::shared_lock lock(mutex_);
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
