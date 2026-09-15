// Kernel dependences
#include <Lattice/Kernel/Node.hpp>

// Plugin dependences
#include "LoaderAPI.hpp"

// Sources
#include "include/SoA.hpp"
#include "include/SoALoader.hpp"
#include "NamedSoA.hpp"
#include "NamedSoALoader.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<StdData::SoA>();
    blueprints.blueprint<SoALoader, LoaderAPI>();
    blueprints.blueprint<StdData::NamedSoA, StdData::SoA>();
    blueprints.blueprint<NamedSoALoader, LoaderAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}