#include <Lattice/Composition.hpp>
#include "src/Verlet.hpp"
#include "src/KDK.hpp"

extern "C" bool plugin_register(Lattice::Blueprints& types) {
    Lattice::BlueprintRegister::object<Integrators::Verlet, ParticleDynamics::IntegratorAPI>(types);
    Lattice::BlueprintRegister::object<Integrators::KDK, ParticleDynamics::IntegratorAPI>(types);
    return true;
}
