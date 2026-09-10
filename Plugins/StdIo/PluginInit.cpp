// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/SubsystemAPI.hpp"

// Plugin dependences

// Sources
#include "IOSubsystem.hpp"
#include "LoaderAPI.hpp"
#include "ParserAPI.hpp"
#include "TomlParser.hpp"
// #include "JsonParser.hpp"
// #include "YamlParser.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<IOSubsystem, SubsystemAPI>();
    blueprints.blueprint<LoaderAPI>();
    blueprints.blueprint<ParserAPI>();
    blueprints.blueprint<TomlParser, ParserAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}