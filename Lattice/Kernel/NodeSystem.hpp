#pragma once

#include "Lattice/Kernel/NodeContext.hpp"
#include "Lattice/Kernel/NodeViews.hpp"
#include "Lattice/Kernel/NodeOps.hpp"
#include "Lattice/Kernel/NodeQuery.hpp"
#include "Lattice/Kernel/NodeFactory.hpp"
#include "Lattice/Kernel/NodeRegistry.hpp"

namespace Lattice {

class NodeSystem {
    Blueprints& blueprints_;
public:
    NodeRegistry registry;
    NodeFactory factory;
    NodeContext context;
    NodeQuery query;
    NodeOps ops;

    NodeSystem(Blueprints& blueprints)
        : blueprints_(blueprints)
        , query(registry, blueprints)
        , ops(registry, blueprints, query, context)
        , factory(registry, blueprints, context, ops, query) 
        , context(registry) {}

    NodeBuildView build(NodeId id) {
        registry.require(id);
        return {id, factory, blueprints_, query};
    }

    NodeConfigureView configure(NodeId id) {
        registry.require(id);
        return {id, query, context};
    }
};

}