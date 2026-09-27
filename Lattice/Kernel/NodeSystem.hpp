#pragma once

#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/Builder.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/NodeOps.hpp>
#include <Lattice/Kernel/Exports.hpp>

class NodeConfigure;

namespace Lattice {

class NodeSystem {
public:
    Blueprints& blueprints;
    NodeRegistry registry;
    NodeFactory factory;
    NodeContext context;
    NodeQuery query;
    Exports exports;
    NodeOps ops;
    Builder builder;

    NodeSystem(Blueprints& blueprints)
        : blueprints(blueprints)
        , factory(*this)
        , context(registry)
        , query(registry, blueprints, context)
        , ops(*this)
        , builder(*this) {}

    ::NodeConfigure configure(NodeId id);
};

}
