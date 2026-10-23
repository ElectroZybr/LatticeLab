#pragma once
#include "Document.hpp"
#include "ParserAPI.hpp"

/// Standalone C++ parser; has no builder/configure/registration hooks.
class TomlParser final : public ParserAPI {
public:
    std::string_view extension() const override { return ".toml"; }
    Document parseFile(const std::filesystem::path& path) const override;
    void writeFile(const std::filesystem::path& path, const Document& document) const override;
};
