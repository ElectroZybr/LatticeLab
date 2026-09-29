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
    void invalidate(NodeId id);
    void maintain();
    void retireBranch(NodeId id);
    size_t collectRetired();
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
    std::vector<NodeId> collectRemovalClosure(NodeId id) const;

private:
    std::vector<std::vector<NodeId>> retiring_;
    std::vector<NodeId> invalidated_;
};

}
