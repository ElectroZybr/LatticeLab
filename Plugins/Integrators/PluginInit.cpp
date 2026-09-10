#include "Plugins/Integrators/src/Verlet.hpp"
#include "Plugins/Integrators/src/KDK.hpp"

#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Tools/Logger.hpp>

#include <ParticleDynamics/include/ParticleAPI.hpp>

namespace Integrators {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<Verlet, ParticleDynamics::IntegratorAPI>();
    blueprints.blueprint<KDK, ParticleDynamics::IntegratorAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}
}