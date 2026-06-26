#pragma once

#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#include "Lattice/Engine/DynamicLibrary.hpp"
#include "Lattice/Engine/PluginHost.hpp"
#include "Lattice/Engine/physics/IForceField.h"
#include "Lattice/Engine/physics/IIntegrator.h"
#include "Lattice/Engine/physics/IThermostat.h"

class PluginLoader {
public:
    PluginLoader()
        : host_{
              ForceField::registry(),
              Integrator::registry(),
              Thermostat::registry(),
          } {}

    explicit PluginLoader(const std::filesystem::path& pluginsDir)
        : PluginLoader() {
        load(pluginsDir);
    }

    int load(const std::filesystem::path& pluginsDir) {
        int loadedCount = 0;

        if (!std::filesystem::exists(pluginsDir) || !std::filesystem::is_directory(pluginsDir)) {
            std::cerr << "[PluginLoader] Plugins directory is missing or not a directory: " << pluginsDir.string() << "\n";
            return 0;
        }

        std::cerr << "[PluginLoader] Scanning: " << pluginsDir.string() << "\n";

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(pluginsDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::filesystem::path libraryPath = entry.path();
            if (libraryPath.extension() != DynamicLibrary::extension()) {
                continue;
            }

            std::cerr << "[PluginLoader] Loading: " << libraryPath.string() << "\n";

            DynamicLibrary library;
            if (!library.open(libraryPath)) {
                std::cerr << "[PluginLoader] Failed to open '" << libraryPath.string() << "': " << library.lastError() << "\n";
                continue;
            }

            PluginInitFn init = library.symbol<PluginInitFn>("plugin_init");
            if (init == nullptr) {
                std::cerr << "[PluginLoader] Missing symbol 'plugin_init' in '" << libraryPath.string() << "': " << library.lastError() << "\n";
                continue;
            }

            if (!init(host_, library.info)) {
                std::cerr << "[PluginLoader] plugin_init failed for '" << libraryPath.string() << "'\n";
                continue;
            }

            std::cerr << "[PluginLoader] Loaded plugin: "
                      << (library.info.name != nullptr && library.info.name[0] != '\0' ? library.info.name : "<unnamed>")
                      << " id="
                      << (library.info.id != nullptr && library.info.id[0] != '\0' ? library.info.id : "<none>")
                      << " version="
                      << (library.info.version != nullptr && library.info.version[0] != '\0' ? library.info.version : "<none>")
                      << "\n";

            loadedPlugins_.push_back(std::move(library));
            ++loadedCount;
        }

        std::cerr << "[PluginLoader] Total loaded: " << loadedCount << "\n";
        return loadedCount;
    }

    const std::vector<DynamicLibrary>& loadedPlugins() const noexcept { return loadedPlugins_; }

private:
    PluginHost host_;
    std::vector<DynamicLibrary> loadedPlugins_;
};
