// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    return false;
}

extern "C" void plugin_shutdown() {}