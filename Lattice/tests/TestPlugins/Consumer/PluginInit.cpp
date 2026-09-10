// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Node.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    return true;
}

extern "C" void plugin_shutdown() {}