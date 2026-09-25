#pragma once

#include <filesystem>
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

    void load(const std::filesystem::path& path ) {
        Logger::info("IOSubsystem", "load from: {}", std::string(path));
        ParserAPI* parser = findParser(path);
        Document doc;
        if (parser) {
            doc = parser->parseFile(path);

            for (LoaderAPI* loader : loaders) {
                const Lattice::Value* section = doc.section(loader->section());

                if (section)
                    loader->load(*section);
            }
        }
    }

    void save() {

    }

    ~IOSubsystem() {

    }

private:
    ParserAPI* findParser(const std::filesystem::path& path) {
        for (ParserAPI* parser : parsers)
            if (parser && parser->extension() == path.extension())
                return parser;
        Logger::warning("IOSubsystem", "Parser for extension {} not found", std::string(path.extension()));
        return nullptr;
    }
    Children<LoaderAPI> loaders;
    Children<ParserAPI> parsers;
};
