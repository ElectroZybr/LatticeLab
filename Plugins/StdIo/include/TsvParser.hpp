#pragma once

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Exception.hpp>

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
        Lattice::Table dataset;
        std::string line;
        size_t lineNumber = 0;

        while (std::getline(file, line)) {
            ++lineNumber;
            if (line.empty())
                continue;
            if (line[0] != '#')
                break;

            const auto metadata = split(std::string_view(line).substr(1));
            if (metadata.size() != 2 || metadata[0].empty())
                throw Lattice::Exception(tag, "Invalid metadata at line {}: '{}'", lineNumber, line);

            if (dataset.contains(metadata[0]))
                throw Lattice::Exception(tag, "Duplicate metadata '{}' at line {}", metadata[0], lineNumber);

            dataset.emplace(metadata[0], parseValue(metadata[1], lineNumber));
        }

        if (file.bad())
            throw Lattice::Exception(tag, "Failed while reading '{}'", path.string());

        if (line.empty()) {
            document.root().emplace("Dataset", std::move(dataset));
            return document;
        }

        const auto headers = split(line);
        if (headers.empty() || (headers.size() == 1 && headers[0].empty()))
            throw Lattice::Exception(tag, "TSV contains no columns at line {}", lineNumber);

        for (size_t i = 0; i < headers.size(); ++i) {
            if (headers[i].empty())
                throw Lattice::Exception(tag, "Empty column name at line {}, column {}", lineNumber, i + 1);
            for (size_t j = 0; j < i; ++j)
                if (headers[i] == headers[j])
                    throw Lattice::Exception(tag, "Duplicate column '{}' at line {}", headers[i], lineNumber);
        }

        Lattice::Array rows;

        while (std::getline(file, line)) {
            ++lineNumber;
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
        document.root().emplace("Dataset", std::move(dataset));

        return document;
    }

private:
    static std::vector<std::string> split(std::string_view line) {
        std::vector<std::string> result;
        size_t begin = 0;

        while (begin < line.size()) {
            while (begin < line.size() && line[begin] == '\t')
                ++begin;

            if (begin == line.size())
                break;

            const size_t end = line.find('\t', begin);

            if (end == std::string_view::npos) {
                result.emplace_back(line.substr(begin));
                break;
            }

            result.emplace_back(line.substr(begin, end - begin));
            begin = end;
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