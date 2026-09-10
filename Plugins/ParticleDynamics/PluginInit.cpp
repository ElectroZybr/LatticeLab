// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

// Plugin dependences

// Sources
#include "include/ParticleAPI.hpp"
#include "src/SpatialGrid.hpp"

namespace ParticleDynamics {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<IntegratorAPI>();
    blueprints.blueprint<ForceFieldAPI>();
    blueprints.blueprint<SpatialIndexAPI>();

    blueprints.blueprint<SpatialGrid, SpatialIndexAPI>();
    blueprints.blueprint<ParticleStorage>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // ParticleDynamics