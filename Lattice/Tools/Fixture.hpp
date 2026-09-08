#pragma once

#include <Lattice/Lattice.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    DLLoader dlLoader;
    PluginManager pluginManager;
    Kernel kernel;
    Node root;

    RuntimeFixture() : root(kernel) 
                     , pluginManager(kernel.blueprints, dlLoader) {
        kernel.blueprints.registerAPI<ServiceAPI>();
        kernel.blueprints.registerAPI<SubsystemAPI>();
        kernel.blueprints.registerComponent<Settings>();
    }
};
}