#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

class NodeSystem;

class NodeOps {
    NodeSystem& nodeSystem_;

public:
    NodeOps(NodeSystem& nodeSystem)
        : nodeSystem_(nodeSystem) {}

    void configureBranch(NodeId id);
    void destroyBranch(NodeId id);
    void clearContents(NodeId id);
    void configure(NodeId id);

    void dumpTree(NodeId id, NodeId highlighted = InvalidNodeId) const;
    void dumpContext() const;

    NodeId resolvePath(NodeId from, std::string_view path) const;
    std::string stringPath(NodeId id) const;

    NodeId root(NodeId id) const;
    bool isUnder(NodeId id, NodeId ancestor) const;

    std::vector<NodeId> collectTree(NodeId id) const;
};

}