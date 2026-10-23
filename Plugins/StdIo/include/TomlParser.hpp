#pragma once

#include <filesystem>
#include <fstream>
#include <chrono>
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
            throw Lattice::Exception<TomlParser>("Failed to parse '{}': {}", path.string(), error.what());
        }

        return document;
    }


    /// Build the complete TOML tree before opening the output; replace only after a successful write.
    void writeFile(const std::filesystem::path& path, const Document& document) const override {
        const auto table = writeObject(document.root());
        auto temporary = path;
        temporary += "." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp";
        try {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            if (!file) throw Lattice::Exception<TomlParser>("Cannot write '{}'", path.string());
            file << toml::toml_formatter{table};
            file.close();
            if (!file) throw Lattice::Exception<TomlParser>("Failed while writing '{}'", path.string());
            std::filesystem::rename(temporary, path);
        } catch (...) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
    }

private:
    static toml::table writeObject(const Lattice::Object& object) {
        toml::table table;
        for (const auto& [key, value] : object)
            std::visit([&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, Lattice::StringPtr> || std::is_same_v<T, Lattice::ArrayPtr> || std::is_same_v<T, Lattice::ObjectPtr>)
                    if (!item) throw Lattice::Exception<TomlParser>("TOML cannot represent null values");
                if constexpr (std::is_same_v<T, Lattice::StringPtr>) table.insert(key, *item);
                else if constexpr (std::is_same_v<T, Lattice::ObjectPtr>) table.insert(key, writeObject(*item));
                else if constexpr (std::is_same_v<T, Lattice::ArrayPtr>) table.insert(key, writeArray(*item));
                else if constexpr (std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4>) {
                    toml::array array;
                    for (int i = 0; i < item.length(); ++i) array.push_back(double(item[i]));
                    table.insert(key, std::move(array));
                } else table.insert(key, item);
            }, static_cast<const Lattice::Value::variant&>(value));
        return table;
    }

    static toml::array writeArray(const Lattice::Array& values) {
        toml::array array;
        for (const auto& value : values)
            std::visit([&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, Lattice::StringPtr> || std::is_same_v<T, Lattice::ArrayPtr> || std::is_same_v<T, Lattice::ObjectPtr>)
                    if (!item) throw Lattice::Exception<TomlParser>("TOML cannot represent null values");
                if constexpr (std::is_same_v<T, Lattice::StringPtr>) array.push_back(*item);
                else if constexpr (std::is_same_v<T, Lattice::ObjectPtr>) array.push_back(writeObject(*item));
                else if constexpr (std::is_same_v<T, Lattice::ArrayPtr>) array.push_back(writeArray(*item));
                else if constexpr (std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4>) {
                    toml::array vector;
                    for (int i = 0; i < item.length(); ++i) vector.push_back(double(item[i]));
                    array.push_back(std::move(vector));
                } else array.push_back(item);
            }, static_cast<const Lattice::Value::variant&>(value));
        return array;
    }
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
