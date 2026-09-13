#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/SubsystemAPI.hpp"
#include "Lattice/Tools/Logger.hpp"
#include <Lattice/Kernel/Node.hpp>

// Source
#include "Document.hpp"
#include "LoaderAPI.hpp"
#include "ParserAPI.hpp"


class IOSubsystem final : public SubsystemAPI {
public:
    explicit IOSubsystem(Lattice::Node& ioBranch) {
        ioBranch.addImpls<LoaderAPI>();
        ioBranch.addImpls<ParserAPI>();
    }

    void configure(Lattice::Node& ioBranch) {
        ioBranch.on("load", [this]() { loadDir("Config"); } );
        loaders = ioBranch.directCollect<LoaderAPI>();
        parsers = ioBranch.directCollect<ParserAPI>();
    }

    void loadDir(const std::filesystem::path& dir) {
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
    }

    void load(const std::filesystem::path& path ) {
        Logger::ok("IOSubsystem", "load from: {}", std::string(path));
        ParserAPI* parser = findParser(path);
        Document doc;
        if (parser)
            doc = parser->parseFile(path);

        for (LoaderAPI* loader : loaders) {
            const Value* data = doc.get(loader->section());
            if (!data)
                continue;

            loader->load(data);
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
    std::vector<LoaderAPI*> loaders;
    std::vector<ParserAPI*> parsers;
};
