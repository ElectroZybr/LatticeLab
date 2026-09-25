#pragma once

#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

class NodeSystem;

class NodeFactory {
    NodeSystem& nodeSystem_;

    NodeId createNode(
        NodeId parent,
        std::string_view name,
        BlueprintId bp,
        NodeKind kind,
        uint64_t discriminator = std::numeric_limits<uint64_t>::max()
    );

    NodeId resource(
        NodeId target,
        BlueprintId api,
        std::string_view instance,
        const void* desc,
        uint64_t discriminator
    );

    NodeId reference(
        NodeId caller,
        NodeId target,
        BlueprintId api,
        std::string_view instance,
        NodeKind kind
    );

public:
    NodeFactory(NodeSystem& nodeSystem)
        : nodeSystem_(nodeSystem) {}

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

    void share(
        NodeId id, 
        BlueprintId api
    );

    NodeId addLocal(
        NodeId caller,
        NodeId target,
        BlueprintId api,
        std::string_view instance,
        const void* desc = nullptr
    );

    NodeId addShare(
        NodeId caller,
        NodeId target,
        BlueprintId api,
        std::string_view instance,
        const void* desc = nullptr
    );
};

}
