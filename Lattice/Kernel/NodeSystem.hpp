#pragma once

#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/NodeOps.hpp>
#include <Lattice/Kernel/Exports.hpp>

class NodeBuild;
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

    NodeSystem(Blueprints& blueprints)
        : blueprints(blueprints)
        , query(registry, blueprints, context)
        , ops(*this)
        , factory(*this) 
        , context(registry) {}

    ::NodeBuild build(NodeId id);
    ::NodeConfigure configure(NodeId id);
};

}