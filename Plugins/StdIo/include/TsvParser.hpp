#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <Lattice/Tools/Exception.hpp>

#include "Document.hpp"
#include "ParserAPI.hpp"

class TsvParser final : public ParserAPI {
    static constexpr std::string_view tag = "TsvParser";

public:
    std::string_view extension() const override { return ".tsv"; }

    Document parseFile(const std::filesystem::path& path) const override {
        std::ifstream file(path);
        if (!file.is_open())
            throw Lattice::Exception(tag, "Failed to open '{}'", path.string());

        Document document;
        Lattice::Object dataset;
        std::string line;
        size_t lineNumber = 0;
        std::string section = "Dataset";

        if (!std::getline(file, line))
            throw Lattice::Exception(tag, "TSV is empty");

        ++lineNumber;
        trimCR(line);

        if (isSection(line)) {
            section = parseSection(line, lineNumber);

            if (!std::getline(file, line))
                throw Lattice::Exception(tag, "TSV section '{}' contains no data", section);

            ++lineNumber;
            trimCR(line);
        }

        while (!line.empty() && line.front() == '#') {
            const auto [key, value] = parseMetadata(line, lineNumber);

            if (dataset.contains(key))
                throw Lattice::Exception(tag, "Duplicate metadata '{}' at line {}", key, lineNumber);

            dataset.emplace(key, parseValue(value, lineNumber));

            if (!std::getline(file, line))
                throw Lattice::Exception(tag, "TSV contains no table");

            ++lineNumber;
            trimCR(line);
        }

        if (line.empty()) {
            if (!std::getline(file, line))
                throw Lattice::Exception(tag, "TSV contains no columns");

            ++lineNumber;
            trimCR(line);
        }

        const auto headers = split(line);
        validateHeaders(headers, lineNumber);

        Lattice::Array rows;

        while (std::getline(file, line)) {
            ++lineNumber;
            trimCR(line);

            if (line.empty())
                continue;

            const auto values = split(line);

            if (values.size() != headers.size())
                throw Lattice::Exception(tag, "Invalid row at line {}: expected {} columns, got {}", lineNumber, headers.size(), values.size());

            Lattice::Array row;
            row.reserve(values.size());

            for (const auto& value : values)
                row.emplace_back(parseValue(value, lineNumber));

            rows.emplace_back(std::move(row));
        }

        if (file.bad())
            throw Lattice::Exception(tag, "Failed while reading '{}'", path.string());

        dataset.emplace("columns", toArray(headers));
        dataset.emplace("rows", std::move(rows));
        document.root().emplace(std::move(section), std::move(dataset));

        return document;
    }

private:
    static bool isSection(std::string_view line) noexcept {
        return line.size() >= 3 && line.front() == '[' && line.back() == ']';
    }

    static std::string parseSection(std::string_view line, size_t lineNumber) {
        const auto name = line.substr(1, line.size() - 2);

        if (name.empty())
            throw Lattice::Exception(tag, "Empty section name at line {}", lineNumber);

        return std::string(name);
    }

    static std::pair<std::string, std::string> parseMetadata(std::string_view line, size_t lineNumber) {
        line.remove_prefix(1);

        const size_t separator = line.find_first_of(" \t");

        if (separator == std::string_view::npos)
            throw Lattice::Exception(tag, "Invalid metadata at line {}: '{}'", lineNumber, line);

        const auto key = line.substr(0, separator);

        size_t begin = separator;
        while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t'))
            ++begin;

        if (key.empty() || begin == line.size())
            throw Lattice::Exception(tag, "Invalid metadata at line {}: '{}'", lineNumber, line);

        return {std::string(key), std::string(line.substr(begin))};
    }

    static void validateHeaders(const std::vector<std::string>& headers, size_t lineNumber) {
        if (headers.empty())
            throw Lattice::Exception(tag, "TSV contains no columns at line {}", lineNumber);

        for (size_t i = 0; i < headers.size(); ++i) {
            if (headers[i].empty())
                throw Lattice::Exception(tag, "Empty column name at line {}, column {}", lineNumber, i + 1);

            for (size_t j = 0; j < i; ++j)
                if (headers[i] == headers[j])
                    throw Lattice::Exception(tag, "Duplicate column '{}' at line {}", headers[i], lineNumber);
        }
    }

    static void trimCR(std::string& line) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
    }

    static std::vector<std::string> split(std::string_view line) {
        std::vector<std::string> result;
        size_t i = 0;

        while (i < line.size()) {
            while (i < line.size() && line[i] == '\t') ++i;
            if (i == line.size()) break;
            const size_t begin = i;
            while (i < line.size() && line[i] != '\t') ++i;
            result.emplace_back(line.substr(begin, i - begin));
        }

        return result;
    }

    static Lattice::Array toArray(const std::vector<std::string>& values) {
        Lattice::Array result;
        result.reserve(values.size());
        for (const auto& value : values)
            result.emplace_back(value);
        return result;
    }

    static Lattice::Value parseValue(std::string_view value, size_t lineNumber) {
        if (value.empty())
            return std::string{};

        try {
            size_t pos = 0;
            const auto integer = std::stoll(std::string(value), &pos);
            if (pos == value.size())
                return int64_t(integer);
        } catch (const std::invalid_argument&) {
        } catch (const std::out_of_range&) {
            throw Lattice::Exception(tag, "Integer value out of range at line {}: '{}'", lineNumber, value);
        }

        try {
            size_t pos = 0;
            const auto floating = std::stod(std::string(value), &pos);
            if (pos == value.size()) {
                if (!std::isfinite(floating))
                    throw Lattice::Exception(tag, "Invalid floating-point value at line {}: '{}'", lineNumber, value);
                return floating;
            }
        } catch (const std::invalid_argument&) {
        } catch (const std::out_of_range&) {
            throw Lattice::Exception(tag, "Floating-point value out of range at line {}: '{}'", lineNumber, value);
        }

        return std::string(value);
    }
};