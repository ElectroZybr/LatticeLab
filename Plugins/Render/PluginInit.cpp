// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>

// Plugin dependences

// Sources
#include "Render.hpp"

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<Render>();
    return true;
}

extern "C" void plugin_shutdown() {

}