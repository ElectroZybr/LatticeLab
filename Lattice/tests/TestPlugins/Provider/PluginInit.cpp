// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

struct TestAPI {
    static constexpr std::string_view apiName = "TestAPI";
};

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<TestAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}