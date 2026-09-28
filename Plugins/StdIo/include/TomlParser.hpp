#pragma once

#include <filesystem>
#include <string>

#include <toml++/toml.hpp>
#include <Lattice/Tools/Exception.hpp>

#include "Document.hpp"
#include "ParserAPI.hpp"


class TomlParser final : public ParserAPI {
public:
    std::string_view extension() const override { return ".toml"; }
    
    Document parseFile(const std::filesystem::path& path) const override {
        Document document;

        try {
            const toml::table table = toml::parse_file(path.string());

            parseTable(table, document.root());
        }
        catch (const toml::parse_error& error) {
            // TODO
        }

        return document;
    }

private:
    static void parseTable(
        const toml::table& table,
        Lattice::Object& output
    ) {
        for (const auto& [key, node] : table) {
            output.emplace(std::string(key.str()), parseValue(node));
        }
    }

    static Lattice::Value parseValue(const toml::node& node) {
        if (const auto* table = node.as_table()) {
            Lattice::Object child;
            parseTable(*table, child);
            return child;
        }

        if (const auto* array = node.as_array()) {
            Lattice::Array values;
            values.reserve(array->size());

            for (const auto& element : *array)
                values.emplace_back(parseValue(element));

            return values;
        }

        if (const auto* value = node.as_string())
            return std::string(value->get());

        if (const auto* value = node.as_integer())
            return int64_t(value->get());

        if (const auto* value = node.as_floating_point())
            return double(value->get());

        if (const auto* value = node.as_boolean())
            return bool(value->get());

        throw Lattice::Exception<TomlParser>("Unsupported TOML value");
    }
};
