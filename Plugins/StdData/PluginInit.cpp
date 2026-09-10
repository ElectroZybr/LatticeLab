// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

#include "LoaderAPI.hpp"

// Sources
#include "include/SoA.hpp"
#include "include/SoALoader.hpp"
// #include "include/CSR.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<StdData::SoA>();
    blueprints.blueprint<SoALoader, LoaderAPI>();
    // reg.registerComponent<CSR>();
    return true;
}

extern "C" void plugin_shutdown() {}