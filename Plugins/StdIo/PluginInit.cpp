// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include "Lattice/Kernel/Blueprints.hpp"
#include "Lattice/Kernel/SubsystemAPI.hpp"

// Plugin dependences

// Sources
#include "IOSubsystem.hpp"
#include "LoaderAPI.hpp"
#include "ParserAPI.hpp"
#include "TomlParser.hpp"
// #include "JsonParser.hpp"
// #include "YamlParser.hpp"


extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    reg.registerImpl<IOSubsystem, SubsystemAPI>();
    reg.registerAPI<LoaderAPI>();
    reg.registerAPI<ParserAPI>();
    reg.registerImpl<TomlParser, ParserAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}