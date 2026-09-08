// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Registry.hpp"
#include "Lattice/Kernel/SubsystemAPI.hpp"

// Sources
#include "InputAPI.hpp"
#include "ActionMap.hpp"


extern "C" bool plugin_register(Lattice::Registry& reg) {
    reg.registerAPI<InputAPI>();
    reg.registerImpl<ActionMap, SubsystemAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}