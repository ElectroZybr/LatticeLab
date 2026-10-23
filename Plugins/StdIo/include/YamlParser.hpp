#pragma once
#include "Document.hpp"
#include "ParserAPI.hpp"

/// YAML mapping/sequence/scalar <-> Document. Quoted scalars stay strings.
/// Both conventional YAML file extensions use the same parser and writer.
class YamlParser final : public ParserAPI {
public:
    std::string_view extension() const override { return ".yaml"; }
    bool supportsExtension(std::string_view extension) const override {
        return extension == ".yaml" || extension == ".yml";
    }
    Document parseFile(const std::filesystem::path& path) const override;
    void writeFile(const std::filesystem::path& path, const Document& document) const override;
};
