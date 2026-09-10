#pragma once

#include <Lattice/Lattice.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    DLLoader dlLoader;
    RuntimeContext run_ctx;
    Node root;
    Node& blueprints;
    PluginManager pluginManager;

    RuntimeFixture()
            : root(run_ctx, nullptr)
            , blueprints(root.addFolder(DefaultBlueprintsPath))
            , pluginManager(blueprints, dlLoader) {
    }
};
}