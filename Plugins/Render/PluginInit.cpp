// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Blueprints.hpp"
#include <Lattice/Kernel/ServiceAPI.hpp>

// Plugin dependences

// Sources
#include "Render.hpp"

extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    reg.registerComponent<Render>();
    return true;
}

extern "C" void plugin_shutdown() {

}