#pragma once
#include <filesystem>
#include <shared_mutex>
#include <vector>
#include "Document.hpp"
#include "ParserAPI.hpp"

/**
 * Ordinary C++ file/format service over a set of parsers. Parsers are borrowed:
 * composition assigns the current set (setParsers) whenever the tree changes.
 * There is no runtime lookup, parser construction or domain-loader discovery.
 *
 * Reads and writes may run on other threads: setParsers waits until running
 * operations finish, so a parser removed from the set is no longer in use.
 */
class IOSubsystem final {
public:
    explicit IOSubsystem(std::vector<ParserAPI*> parsers = {});
    Document read(const std::filesystem::path& path) const;
    void write(const std::filesystem::path& path, const Document& document) const;

    void setParsers(std::vector<ParserAPI*> parsers);

private:
    ParserAPI& requireParser(const std::filesystem::path& path) const;
    mutable std::shared_mutex mutex_;
    std::vector<ParserAPI*> parsers_;
};
