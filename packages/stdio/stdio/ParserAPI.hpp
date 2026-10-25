#pragma once

#include <Lattice/tools/Exception.hpp>

#include <filesystem>
#include <string_view>

class Document;

/// Plain C++ format contract. Registration and ownership belong to composition.
class ParserAPI {
public:
    virtual ~ParserAPI() = default;
    virtual std::string_view extension() const = 0;
    virtual bool supportsExtension(std::string_view value) const { return value == extension(); }
    virtual Document parseFile(const std::filesystem::path& path) const = 0;
    /// Formats without a writer (for example TSV datasets) remain read-only.
    virtual void writeFile(const std::filesystem::path& path, const Document&) const {
        throw Lattice::Exception<ParserAPI>("Writing '{}' is not supported", path.string());
    }
};
