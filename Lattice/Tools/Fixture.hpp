#pragma once

#include <Lattice/Lattice.hpp>
#include <Lattice/Kernel/Model.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    DLLoader dlLoader;
    Context run_ctx;
    Node root;
    Blueprints& blueprints;
    PluginManager pluginManager;

    RuntimeFixture()
            : root(run_ctx, nullptr)
            , blueprints(run_ctx.blueprints)
            , pluginManager(blueprints, dlLoader) {
        blueprints.add<ServiceAPI>();
        blueprints.add<SubsystemAPI>();
        blueprints.add<Model, ServiceAPI>();
    }
};
}