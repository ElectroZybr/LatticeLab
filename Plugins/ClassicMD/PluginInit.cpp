// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/Model.hpp>

// Plugin dependences
#include <ParticleDynamics/include/ParticleAPI.hpp>

// Source
#include "src/ClassicMD.hpp"

namespace ClassicMD {

extern "C" bool plugin_register(Lattice::Registry& reg) {
    reg.registerImpl<ClassicMD, Model, ServiceAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // namespace ClassicMD