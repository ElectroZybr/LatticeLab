#pragma once

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Tools/Exception.hpp>

#include <filesystem>
#include <string_view>

class Document;

class ParserAPI : public Lattice::Component {
public:
    virtual std::string_view extension() const = 0;
    virtual Document parseFile(const std::filesystem::path& path) const = 0;
    /// Formats without a writer (for example TSV datasets) remain read-only.
    virtual void writeFile(const std::filesystem::path& path, const Document&) const {
        throw Lattice::Exception<ParserAPI>("Writing '{}' is not supported", path.string());
    }
};