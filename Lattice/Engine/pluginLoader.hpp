#pragma once

#include <filesystem>
#include <utility>
#include <vector>

#include "Lattice/Engine/DynamicLibrary.hpp"
#include "Lattice/Engine/PluginHost.hpp"
#include "Lattice/Log.hpp"
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
            Log::warning("PluginLoader", "Plugins directory is missing or not a directory: {}", pluginsDir.string());
            return 0;
        }

        Log::write("PluginLoader", "Scanning {}...", pluginsDir.string());

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(pluginsDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::filesystem::path libraryPath = entry.path();
            if (libraryPath.extension() != DynamicLibrary::extension()) {
                continue;
            }

            Log::write("PluginLoader", "Loading {}", libraryPath.string());

            DynamicLibrary library;
            if (!library.open(libraryPath)) {
                Log::error("PluginLoader", "Failed to open {}: {}",
                           "'{}'", libraryPath.string(),
                           Log::highlight(library.lastError()));
                continue;
            }

            PluginInitFn init = library.symbol<PluginInitFn>("plugin_init");
            if (init == nullptr) {
                Log::error("PluginLoader", "Missing symbol {} in {}: {}",
                           Log::highlight("'plugin_init'"),
                           "'{}'", libraryPath.string(),
                           Log::highlight(library.lastError()));
                continue;
            }

            if (!init(host_, library.info)) {
                Log::error("PluginLoader", "plugin_init failed for '{}'", libraryPath.string());
                continue;
            }

            Log::ok(
                "PluginLoader",
                "Loaded \"{}\" id={} version={}",
                Log::highlight(library.info.name != nullptr && library.info.name[0] != '\0' ? library.info.name : "<unnamed>"),
                Log::highlight(library.info.id != nullptr && library.info.id[0] != '\0' ? library.info.id : "<none>"),
                Log::highlight(library.info.version != nullptr && library.info.version[0] != '\0' ? library.info.version : "<none>"));

            loadedPlugins_.push_back(std::move(library));
            ++loadedCount;
        }

        if (loadedCount == 0) {
            Log::warning("PluginLoader", "No plugins were loaded from {}", pluginsDir.string());
        } else {
            Log::ok("PluginLoader", "Loaded {} plugin", Log::highlight("{}", loadedCount));
        }
        return loadedCount;
    }

    const std::vector<DynamicLibrary>& loadedPlugins() const noexcept { return loadedPlugins_; }

private:
    PluginHost host_;
    std::vector<DynamicLibrary> loadedPlugins_;
};
