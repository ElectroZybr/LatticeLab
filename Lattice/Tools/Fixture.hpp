#pragma once

#include <Lattice/Kernel/Model.hpp>
#include "Lattice/Kernel/BlueprintRegister.hpp"
#include "Lattice/Kernel/BasicTable.hpp"
#include "Lattice/Kernel/Context.hpp"
#include "Lattice/Kernel/DLLoader.hpp"
#include "Lattice/Kernel/PluginManager.hpp"

namespace Lattice {

struct Fixture {
    using Factory = std::unique_ptr<Fixture> (*)(size_t);
    virtual ~Fixture() = default;

    // быстрый сброс тестируемого состояния, 
    // внутри пересоздание/очистка/копирование из буфера начального состояния
    // вызывается между итерациями в тестах/бенчмарках
    virtual void prepare() {};
};

struct RuntimeFixture : public Fixture {
    Context run_ctx;
    NodeId root = InvalidNodeId;
    DLLoader dlLoader;
    PluginManager pluginManager;

    ~RuntimeFixture() override { run_ctx.nodes.ops.destroyBranch(root); }

    RuntimeFixture() : pluginManager(run_ctx.blueprints, dlLoader) {
        BlueprintRegister::add<Component>(run_ctx.blueprints);
        BlueprintRegister::add<Table, Component>(run_ctx.blueprints);
        BlueprintRegister::add<BasicTable, Table>(run_ctx.blueprints);
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
