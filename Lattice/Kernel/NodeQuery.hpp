#pragma once

#include <string_view>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

class NodeRegistry;
class Blueprints;
class NodeContext;

class NodeQuery {
    NodeRegistry& registry_;
    Blueprints& blueprints_;
    NodeContext& context_;

    bool provides(NodeId id, BlueprintId api) const;

public:
    NodeQuery(NodeRegistry& registry, Blueprints& blueprints, NodeContext& context)
        : registry_(registry), blueprints_(blueprints), context_(context) {}

    NodeId find(
        NodeId from, 
        BlueprintId api, 
        std::string_view instance = DefaultInstanceName
    ) const;

    NodeId require(
        NodeId from, 
        BlueprintId api, 
        std::string_view instance = DefaultInstanceName
    ) const;

    NodeId find(
        NodeId from, 
        std::string_view api, 
        std::string_view instance = DefaultInstanceName
    ) const;

    NodeId require(
        NodeId from, 
        std::string_view api, 
        std::string_view instance = DefaultInstanceName
    ) const;

    NodeId shared(
        NodeId from, 
        NodeId api, 
        std::string_view instance = DefaultInstanceName
    ) const;

    std::vector<NodeId> collect(NodeId from, BlueprintId api) const;
    std::vector<NodeId> collect(NodeId from, std::string_view api) const;
    void* resolve(NodeId id, BlueprintId api) const;
    void* resolve(NodeId id, std::string_view api) const;
};

}