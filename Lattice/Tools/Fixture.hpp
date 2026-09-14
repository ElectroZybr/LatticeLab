#pragma once

#include <Lattice/Lattice.hpp>
#include <Lattice/Kernel/Model.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    DLLoader dlLoader;
    Context run_ctx;
    Node root;
    Node& blueprints;
    PluginManager pluginManager;

    RuntimeFixture()
            : root(run_ctx, nullptr)
            , blueprints(root.addFolder(DefaultBlueprintsPath))
            , pluginManager(blueprints, dlLoader) {
        blueprints.blueprint<ServiceAPI>();
        blueprints.blueprint<SubsystemAPI>();
        blueprints.blueprint<Model, ServiceAPI>();
    }
};
}