#include <Lattice/Kernel/Blueprints.hpp>

extern "C" bool plugin_register(Lattice::Blueprints& blueprints) {
    blueprints.add("Failed::PartialRegistration");
    return false;
}
