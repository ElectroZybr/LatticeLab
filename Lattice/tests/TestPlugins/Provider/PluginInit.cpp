// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Blueprints.hpp"


struct TestAPI {
    static constexpr std::string_view apiName = "TestAPI";
};

extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    reg.registerAPI<TestAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}