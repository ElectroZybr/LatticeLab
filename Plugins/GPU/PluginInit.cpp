// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

// Sources
#include "include/WGPU.hpp"

namespace GPU {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<WGPU>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // GPU