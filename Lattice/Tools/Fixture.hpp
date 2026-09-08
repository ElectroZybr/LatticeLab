#pragma once

#include <Lattice/Lattice.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    DLLoader dlLoader;
    PluginManager pluginManager;
    Kernel kernel;
    Node root;

    RuntimeFixture() : root(kernel) 
                     , pluginManager(kernel.registry, dlLoader) {
        kernel.registry.registerAPI<ServiceAPI>();
        kernel.registry.registerAPI<SubsystemAPI>();
        kernel.registry.registerComponent<Settings>();
    }
};
}