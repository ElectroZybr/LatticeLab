// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

// Sources
#include "Lattice/Kernel/Blueprints.hpp"
#include "include/WGPU.hpp"

namespace GPU {

extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    reg.registerComponent<WGPU>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // GPU