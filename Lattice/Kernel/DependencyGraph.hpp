#pragma once

#include <span>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

class DependencyGraph {
public:
    void add(NodeId dependent, NodeId dependency);
    void clearDependencies(NodeId node);
    void removeNode(NodeId node);
    void clear();

    std::span<const NodeId> dependencies(NodeId node) const;
    std::span<const NodeId> dependents(NodeId node) const;

private:
    void ensure(NodeId node);

    std::vector<std::vector<NodeId>> dependencies_;
    std::vector<std::vector<NodeId>> dependents_;
};

}
