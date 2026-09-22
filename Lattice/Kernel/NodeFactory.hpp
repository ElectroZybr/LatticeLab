#pragma once

#include <Lattice/Kernel/Ids.hpp>

namespace Lattice {

class NodeRegistry;
class Blueprints;

class NodeFactory {
    NodeRegistry& nodes_;
    Blueprints& blueprints_;

    NodeId createNode(NodeId parent, std::string_view name, BlueprintId bp, NodeKind kind);
public:
    NodeFactory(NodeRegistry& nodes, Blueprints& blueprints)
        : nodes_(nodes), blueprints_(blueprints) {}

    NodeId folder(NodeId parent, std::string_view name);

    NodeId component(
        NodeId parent,
        BlueprintId blueprint,
        std::string_view instance = DefaultInstanceName,
        const void* desc = nullptr
    );

    NodeId component(
        NodeId parent, 
        std::string_view blueprint, 
        std::string_view instance = DefaultInstanceName, 
        const void* desc = nullptr);
    
    NodeId slot(
        NodeId parent,
        BlueprintId api,
        std::string_view instance = DefaultInstanceName
    );

    NodeId slot(
        NodeId parent, 
        std::string_view api, 
        std::string_view instance = DefaultInstanceName);

    NodeId binding(
        NodeId parent,
        std::string_view name
    );

    NodeId mount(
        NodeId parent,
        BlueprintId blueprint,
        std::string_view instance = DefaultInstanceName
    );
};

}