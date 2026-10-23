#pragma once

#include <filesystem>
#include <optional>
#include <string>

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/Consts.hpp>
#include "Lattice/Tools/LogMode.hpp"
#include "Lattice/Tools/LogScope.hpp"
#include "Lattice/Tools/Logger.hpp"
#include <Lattice/Kernel/NodeViews.hpp>

// Source
#include "Document.hpp"
#include "LoaderAPI.hpp"
#include "ParserAPI.hpp"


class IOSubsystem final : public Lattice::SubsystemAPI {
public:
    explicit IOSubsystem(NodeBuild ioBranch) {
        loaders = ioBranch.addImpls<LoaderAPI>();
        parsers = ioBranch.addImpls<ParserAPI>();

        ioBranch.action<std::optional<std::string>>(
            "load",
            [this](std::optional<std::string> path) {
                const std::filesystem::path target = path.value_or("Config");
                if (std::filesystem::is_directory(target))
                    loadDir(target);
                else
                    load(target);
            }
        );
    }

    void loadDir(const std::filesystem::path& dir) {
        LogScope loadScope("IOSubsystem", LogMode::Verbose, "load dir: {}", std::string(dir));
        if (!std::filesystem::is_directory(dir)) {
            Logger::warning("IOSubsystem", "config directory '{}' not found", dir.string());
            return;
        }

        for (const std::filesystem::directory_entry& entry :
            std::filesystem::recursive_directory_iterator(dir))
        {
            if (!entry.is_regular_file())
                continue;

            load(entry.path());
        }
        loadScope.finish();
    }

    /// File access stays in StdIo; clients work with the existing Document model.
    Document read(const std::filesystem::path& path) const {
        return requireParser(path).parseFile(path);
    }

    void write(const std::filesystem::path& path, const Document& document) const {
        requireParser(path).writeFile(path, document);
    }

    void load(const std::filesystem::path& path) {
        Logger::info("IOSubsystem", "load from: {}", path.string());
        Document doc = read(path);
        for (LoaderAPI* loader : loaders) {
            const Lattice::Value* section = doc.section(loader->section());
            if (section) loader->load(*section);
        }
    }

    ~IOSubsystem() {

    }

private:
    ParserAPI& requireParser(const std::filesystem::path& path) const {
        for (ParserAPI* parser : parsers)
            if (parser && parser->extension() == path.extension())
                return *parser;
        throw Lattice::Exception<IOSubsystem>("Parser for '{}' not found", path.extension().string());
    }
    Children<LoaderAPI> loaders;
    Children<ParserAPI> parsers;
};
