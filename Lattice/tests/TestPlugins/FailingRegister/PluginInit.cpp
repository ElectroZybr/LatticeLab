// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Blueprints.hpp"


extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    return false;
}

extern "C" void plugin_shutdown() {}