#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/DLLoader.hpp>
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Node.hpp"

namespace Lattice {

struct PluginManifest;

class PluginManager {
    static constexpr std::string_view tag = "PluginManager";
public:
    PluginManager(Node& blueprints, DLLoader& dlLoader)
        : blueprints(blueprints), dlLoader(dlLoader) {}

    uint16_t load(std::filesystem::path path);
    
    void scanDirectory(std::filesystem::path path);
    void checkCandidates();
    uint16_t loadCandidates();

    ~PluginManager();

    const Plugin* findCandidate(std::string_view id) const;
    const std::vector<Plugin*>& queue() const { return loadQueue; }

private:
    PluginManifest parseManifest(std::filesystem::path path);
    bool canLoad(const std::string& id);
    bool prepareLoad(const std::string& id);
    bool loadPlugin(Plugin* candidate);

    std::unordered_map<std::string, Plugin> candidates;
    std::vector<Plugin*> loadQueue;

    Node& blueprints;
    DLLoader& dlLoader;
};
}