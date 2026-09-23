#pragma once

#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

class NodeRegistry;
class NodeFocus;
class Blueprints;
class NodeContext;
class NodeOps;
class NodeQuery;

class NodeFactory {
    NodeRegistry& nodes_;
    Blueprints& blueprints_;
    NodeContext& context_;
    NodeOps& ops_;
    NodeQuery& query_;

    NodeId createNode(NodeId parent, std::string_view name, BlueprintId bp, NodeKind kind);
public:
    NodeFactory(NodeRegistry& nodes, Blueprints& blueprints, NodeContext& context, NodeOps& ops, NodeQuery& query)
        : nodes_(nodes), blueprints_(blueprints), context_(context), ops_(ops), query_(query) {}

    NodeId folder(NodeId parent, std::string_view name);

    void* resolve(NodeId id, BlueprintId api) const;

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

    void choice(
        NodeId id, 
        BlueprintId impl
    );

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
