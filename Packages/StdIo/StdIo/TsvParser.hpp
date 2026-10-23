#pragma once
#include "Document.hpp"
#include "ParserAPI.hpp"

/// Standalone C++ parser; has no builder/configure/registration hooks.
class TsvParser final : public ParserAPI {
public:
    std::string_view extension() const override { return ".tsv"; }
    Document parseFile(const std::filesystem::path& path) const override;
};
