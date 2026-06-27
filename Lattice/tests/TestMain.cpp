#include <iostream>
#include <filesystem>

#include "Lattice/Engine/pluginLoader.hpp"

namespace {
    const std::filesystem::path pluginsPath = std::filesystem::path("Plugins");
}

void runNeighborSearchTests();
void runPhysicsTests();
void runIoTests();

int main() {
    PluginLoader pluginLoader;
    pluginLoader.load(pluginsPath);
    runNeighborSearchTests();
    runPhysicsTests();
    runIoTests();
    std::cout << "All Lattice tests passed." << std::endl;
    return 0;
}
