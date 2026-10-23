#include "YamlParser.hpp"
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <regex>

namespace {
using namespace Lattice;
using Error = Exception<YamlParser>;

Value scalar(const YAML::Node& node) {
    const auto& text = node.Scalar();
    const auto& tag = node.Tag();
    if (tag == "!" || tag == "tag:yaml.org,2002:str") return text;
    const bool implicit = tag.empty() || tag == "?";
    if (implicit || tag == "tag:yaml.org,2002:bool") {
        if (text == "true" || text == "True" || text == "TRUE") return true;
        if (text == "false" || text == "False" || text == "FALSE") return false;
        if (!implicit) throw Error("Invalid YAML boolean '{}'", text);
    }
    if (implicit || tag == "tag:yaml.org,2002:int") {
        static const std::regex integer{R"([-+]?(0x[0-9a-fA-F]+|0o[0-7]+|[0-9]+))"};
        if (std::regex_match(text, integer)) {
            std::string digits = text;
            const bool negative = digits.front() == '-';
            if (digits.front() == '+' || negative) digits.erase(0, 1);
            int base = 10;
            if (digits.starts_with("0x")) { base = 16; digits.erase(0, 2); }
            else if (digits.starts_with("0o")) { base = 8; digits.erase(0, 2); }
            if (negative) digits.insert(digits.begin(), '-');
            int64_t result;
            const auto conversion = std::from_chars(digits.data(), digits.data() + digits.size(), result, base);
            if (conversion.ec != std::errc{} || conversion.ptr != digits.data() + digits.size())
                throw Error("YAML integer '{}' is outside int64 range", text);
            return result;
        }
        if (!implicit) throw Error("Invalid YAML integer '{}'", text);
    }
    if (implicit || tag == "tag:yaml.org,2002:float") {
        if (text == ".inf" || text == ".Inf" || text == ".INF" || text == "+.inf" || text == "+.Inf" || text == "+.INF")
            return std::numeric_limits<double>::infinity();
        if (text == "-.inf" || text == "-.Inf" || text == "-.INF") return -std::numeric_limits<double>::infinity();
        if (text == ".nan" || text == ".NaN" || text == ".NAN") return std::numeric_limits<double>::quiet_NaN();
        static const std::regex floating{R"([-+]?(\.[0-9]+|[0-9]+(\.[0-9]*)?)([eE][-+]?[0-9]+)?)"};
        if (std::regex_match(text, floating)) {
            const auto view = std::string_view(text).substr(text.front() == '+' ? 1 : 0);
            double result;
            const auto conversion = std::from_chars(view.data(), view.data() + view.size(), result);
            if (conversion.ec != std::errc{} || conversion.ptr != view.data() + view.size())
                throw Error("YAML number '{}' is outside double range", text);
            return result;
        }
        if (!implicit) throw Error("Invalid YAML floating-point value '{}'", text);
    }
    if (!implicit) throw Error("Unsupported YAML tag '{}'", tag);
    return text;
}

Value parse(const YAML::Node& node, std::vector<YAML::Node>& ancestors) {
    if (node.IsNull()) throw Error("YAML null has no representation in Lattice::Value");
    if (node.IsScalar()) return scalar(node);
    if (std::ranges::any_of(ancestors, [&](const auto& parent) { return parent.is(node); }))
        throw Error("Cyclic YAML aliases cannot be represented by a Document");
    ancestors.push_back(node);
    Value result;
    if (node.IsSequence()) {
        Array array;
        for (const auto& item : node) array.push_back(parse(item, ancestors));
        result = std::move(array);
    } else if (node.IsMap()) {
        Object object;
        for (const auto& item : node) {
            if (!item.first.IsScalar()) throw Error("YAML mapping keys must be strings");
            const auto key = item.first.Scalar();
            if (!object.emplace(key, parse(item.second, ancestors)).second)
                throw Error("Duplicate YAML key '{}'", key);
        }
        result = std::move(object);
    } else throw Error("Unsupported YAML node");
    ancestors.pop_back();
    return result;
}

// Formatting doubles with a decimal/exponent keeps nested Value arrays/objects
// from silently turning 1.0 into an integer on the next load.
void emitNumber(YAML::Emitter& out, double value) {
    if (std::isnan(value)) { out << YAML::Auto << ".nan"; return; }
    if (std::isinf(value)) { out << YAML::Auto << (value < 0 ? "-.inf" : ".inf"); return; }
    auto text = std::format("{:.17g}", value);
    if (text.find_first_of(".eE") == std::string::npos) text += ".0";
    out << YAML::Auto << text;
}

void emit(YAML::Emitter& out, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, StringPtr> || std::is_same_v<T, ArrayPtr> || std::is_same_v<T, ObjectPtr>)
            if (!item) throw Error("Null values cannot be written as Lattice YAML");
        if constexpr (std::is_same_v<T, StringPtr>) out << YAML::DoubleQuoted << *item;
        else if constexpr (std::is_same_v<T, ObjectPtr>) {
            out << YAML::BeginMap;
            // Stable output makes configuration diffs readable.
            std::vector<std::string> keys;
            for (const auto& [key, unused] : *item) keys.push_back(key);
            std::ranges::sort(keys);
            for (const auto& key : keys) {
                out << YAML::Key << YAML::DoubleQuoted << key << YAML::Value;
                emit(out, item->at(key));
            }
            out << YAML::EndMap;
        } else if constexpr (std::is_same_v<T, ArrayPtr>) {
            out << YAML::BeginSeq;
            for (const auto& element : *item) emit(out, element);
            out << YAML::EndSeq;
        } else if constexpr (std::is_same_v<T, double>) emitNumber(out, item);
        else if constexpr (std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4>) {
            out << YAML::Flow << YAML::BeginSeq;
            for (int i = 0; i < item.length(); ++i) emitNumber(out, item[i]);
            out << YAML::EndSeq;
        } else out << YAML::Auto << item;
    }, static_cast<const Value::variant&>(value));
}
}

Document YamlParser::parseFile(const std::filesystem::path& path) const {
    try {
        const auto documents = YAML::LoadAllFromFile(path.string());
        if (documents.size() != 1 || !documents.front().IsMap())
            throw Error("Expected one YAML document with a mapping root");
        std::vector<YAML::Node> ancestors;
        return Document{parse(documents.front(), ancestors).get<Lattice::Object>()};
    } catch (const std::exception& error) {
        throw Error("Failed to read YAML '{}': {}", path.string(), error.what());
    }
}

void YamlParser::writeFile(const std::filesystem::path& path, const Document& document) const {
    YAML::Emitter output;
    emit(output, Lattice::Value{document.root()});
    if (!output.good()) throw Error("Failed to format YAML '{}': {}", path.string(), output.GetLastError());
    auto temporary = path;
    temporary += "." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp";
    try {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) throw Error("Cannot write '{}'", path.string());
        file << output.c_str() << '\n';
        file.close();
        if (!file) throw Error("Failed while writing '{}'", path.string());
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}
