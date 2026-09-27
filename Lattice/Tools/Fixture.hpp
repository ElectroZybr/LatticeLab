#pragma once

#include <Lattice/Lattice.hpp>
#include <Lattice/Kernel/Model.hpp>

namespace Lattice {


struct RuntimeFixture : public TestFixture {
    Context run_ctx;
    NodeId root = InvalidNodeId;
    DLLoader dlLoader;
    PluginManager pluginManager;

    ~RuntimeFixture() override { run_ctx.nodes.ops.destroyBranch(root); }

    RuntimeFixture() : pluginManager(run_ctx.blueprints, dlLoader) {
        BlueprintRegister::add<Component>(run_ctx.blueprints);
        BlueprintRegister::add<ServiceAPI>(run_ctx.blueprints);
        BlueprintRegister::add<SubsystemAPI>(run_ctx.blueprints);
        BlueprintRegister::add<Model, ServiceAPI>(run_ctx.blueprints);
        root = run_ctx.nodes.factory.folder(InvalidNodeId, "Root");
    }

    ::NodeBuild build(NodeId id) {
        run_ctx.nodes.registry.require(id);
        return ::NodeBuild{id, run_ctx.nodes};
    }
};
}
