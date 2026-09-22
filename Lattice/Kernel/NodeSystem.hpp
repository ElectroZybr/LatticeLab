#pragma once

#include "Lattice/Kernel/NodeOps.hpp"
#include "Lattice/Kernel/NodeQuery.hpp"
#include "Lattice/Kernel/NodeFactory.hpp"
#include "Lattice/Kernel/NodeRegistry.hpp"

namespace Lattice {

class NodeSystem {
    Context* context_;
public:
    NodeRegistry registry;
    NodeFactory factory;
    NodeQuery query;
    NodeOps ops;

    NodeSystem(Blueprints& blueprints, Context& context)
        : context_(&context), 
          factory(registry, blueprints),
          query(registry, blueprints),
          ops(registry, blueprints, query, context) {}

    NodeBuildView build(NodeId id) {
        registry.require(id);
        return {id, factory};
    }

    NodeConfigureView configure(NodeId id) {
        registry.require(id);
        return {id, query};
    }
};

}