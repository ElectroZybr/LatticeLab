#include <utility>

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/Builder.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeSystem.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace Lattice {

Builder::Batch::Batch(Batch&& other) noexcept
    : builder_(std::exchange(other.builder_, nullptr)),
      branches_(std::move(other.branches_)),
      finished_(std::exchange(other.finished_, true)) {}

Builder::Batch::~Batch() {
    rollback();
}

NodeId Builder::Batch::add(
    NodeId parent,
    BlueprintId blueprint,
    std::string_view instance,
    const void* descriptor
) {
    if (!builder_ || finished_)
        throw Exception<Builder>("Cannot add a branch to a finished batch");

    NodeId branch = InvalidNodeId;
    try {
        branch = builder_->create(parent, blueprint, instance, descriptor);
        branches_.push_back(branch);
        return branch;
    } catch (...) {
        if (branch != InvalidNodeId) {
            try {
                builder_->nodeSystem_.ops.destroyBranch(branch);
            } catch (...) {
                // Preserve the exception that interrupted add().
            }
        }
        rollback();
        throw;
    }
}

void Builder::Batch::commit() {
    if (!builder_ || finished_)
        throw Exception<Builder>("Cannot commit a finished batch");

    try {
        for (NodeId branch : branches_)
            builder_->nodeSystem_.ops.configureBranch(branch);

        for (NodeId branch : branches_) {
            const auto* node = builder_->nodeSystem_.registry.get(branch);
            if (node && node->parent != InvalidNodeId)
                builder_->nodeSystem_.ops.invalidate(node->parent);
        }
        finished_ = true;
    } catch (...) {
        rollback();
        throw;
    }
}

void Builder::Batch::rollback() noexcept {
    if (!builder_ || finished_)
        return;

    for (auto it = branches_.rbegin(); it != branches_.rend(); ++it) {
        try {
            builder_->nodeSystem_.ops.destroyBranch(*it);
        } catch (...) {
            // Rollback must remain safe during stack unwinding.
        }
    }

    branches_.clear();
    finished_ = true;
}

NodeId Builder::add(
    NodeId parent,
    BlueprintId blueprint,
    std::string_view instance,
    const void* descriptor
) {
    Batch batch = begin();
    const NodeId branch = batch.add(parent, blueprint, instance, descriptor);
    batch.commit();
    return branch;
}

void Builder::del(NodeId parent, NodeId child) {
    const auto& owner = nodeSystem_.registry.require(parent);
    const auto& target = nodeSystem_.registry.require(child);

    if (owner.state == NodeState::Retiring)
        throw Exception<Builder>("Cannot delete a child of retiring node #{}", parent);

    if (target.parent != parent)
        throw Exception<Builder>("Node #{} is not a direct child of node #{}", child, parent);

    if (target.state == NodeState::Retiring)
        throw Exception<Builder>("Node #{} is already retiring", child);

    nodeSystem_.ops.retireBranch(child);
}

::NodeBuild Builder::node(NodeId id) {
    nodeSystem_.registry.require(id);
    return ::NodeBuild{id, nodeSystem_};
}

NodeId Builder::create(
    NodeId parent,
    BlueprintId blueprint,
    std::string_view instance,
    const void* descriptor
) {
    if (parent != InvalidNodeId)
        nodeSystem_.registry.require(parent);
    nodeSystem_.blueprints.require(blueprint);

    const BlueprintId implementation = nodeSystem_.blueprints.resolveImplementation(blueprint);
    if (nodeSystem_.registry.find(instance, parent, implementation) != InvalidNodeId)
        throw Exception<Builder>(
            "Branch '{}:{}' already exists under node #{}",
            nodeSystem_.blueprints.require(implementation).shortName(),
            instance,
            parent
        );

    return nodeSystem_.factory.component(parent, blueprint, instance, descriptor);
}

}
