#pragma once
#include <filesystem>
#include <vector>
#include "Document.hpp"
#include "ParserAPI.hpp"

/**
 * Ordinary C++ file/format service with an explicit, fixed parser set.
 * Parsers are borrowed and must outlive this object. There is no runtime lookup,
 * parser construction, domain-loader discovery, or retained build capability.
 */
class IOSubsystem final {
public:
    explicit IOSubsystem(std::vector<ParserAPI*> parsers);
    Document read(const std::filesystem::path& path) const;
    void write(const std::filesystem::path& path, const Document& document) const;

private:
    ParserAPI& requireParser(const std::filesystem::path& path) const;
    std::vector<ParserAPI*> parsers_;
};
