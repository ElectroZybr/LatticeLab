// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/Model.hpp>

// Plugin dependences
#include "AtomData.hpp"
#include "AtomStorage.hpp"
#include "NamedSoA.hpp"
#include "ParticleStorage.hpp"

// Source
#include "src/ClassicMD.hpp"

namespace ClassicMD {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<ClassicMD, Model>();
    blueprints.blueprint<AtomData, StdData::NamedSoA>();
    blueprints.blueprint<AtomStorage, ParticleDynamics::ParticleStorage>();
    return true;
}

extern "C" void plugin_shutdown() {}
} // namespace ClassicMD