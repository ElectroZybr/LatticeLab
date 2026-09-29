#include <algorithm>

#include <Lattice/Kernel/DependencyGraph.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Lattice {

void DependencyGraph::ensure(NodeId node) {
    if (node == InvalidNodeId)
        throw Exception<DependencyGraph>("Invalid node id");

    const size_t size = static_cast<size_t>(node) + 1;
    if (dependencies_.size() < size) {
        dependencies_.resize(size);
        dependents_.resize(size);
    }
}

void DependencyGraph::add(NodeId dependent, NodeId dependency) {
    ensure(dependent);
    ensure(dependency);

    if (dependent == dependency)
        return;

    auto& dependencies = dependencies_[dependent];
    if (std::ranges::find(dependencies, dependency) != dependencies.end())
        return;

    dependencies.push_back(dependency);
    dependents_[dependency].push_back(dependent);
}

void DependencyGraph::clearDependencies(NodeId node) {
    if (node >= dependencies_.size())
        return;

    for (NodeId dependency : dependencies_[node])
        std::erase(dependents_[dependency], node);

    dependencies_[node].clear();
}

void DependencyGraph::removeNode(NodeId node) {
    if (node >= dependencies_.size())
        return;

    clearDependencies(node);

    for (NodeId dependent : dependents_[node])
        std::erase(dependencies_[dependent], node);

    dependents_[node].clear();
}

void DependencyGraph::clear() {
    dependencies_.clear();
    dependents_.clear();
}

std::span<const NodeId> DependencyGraph::dependencies(NodeId node) const {
    return node < dependencies_.size()
        ? std::span<const NodeId>{dependencies_[node]}
        : std::span<const NodeId>{};
}

std::span<const NodeId> DependencyGraph::dependents(NodeId node) const {
    return node < dependents_.size()
        ? std::span<const NodeId>{dependents_[node]}
        : std::span<const NodeId>{};
}

}
