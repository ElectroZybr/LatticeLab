// Kernel dependences
#include "AtomData.hpp"
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/Model.hpp>

// Plugin dependences
#include <ParticleDynamics/include/ParticleAPI.hpp>

// Source
#include "src/ClassicMD.hpp"

namespace ClassicMD {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<ClassicMD, Model>();
    blueprints.blueprint<AtomData>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // namespace ClassicMD