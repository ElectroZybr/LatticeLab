#include <Lattice/Kernel/TreeView.hpp>

#include <format>

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/NodeSystem.hpp>

namespace Lattice {

bool TreeView::contains(NodeId id) const {
    return nodes_ && nodes_->registry.get(id);
}

NodeId TreeView::root(NodeId from) const {
    return nodes_ ? nodes_->ops.root(from) : InvalidNodeId;
}

NodeId TreeView::parent(NodeId node) const {
    return nodes_ ? nodes_->registry.require(node).parent : InvalidNodeId;
}

std::vector<NodeId> TreeView::children(NodeId node) const {
    if (!nodes_)
        return {};

    const auto children = nodes_->registry.children(node);
    return {children.begin(), children.end()};
}

TreeTraversal::Iterator::Iterator(NodeSystem* nodes, NodeId root)
    : nodes_(nodes) {
    if (!nodes_ || !nodes_->registry.get(root))
        return;

    stack_.push_back({root, 0});
    current_ = {root, 0};
}

TreeTraversal::Iterator& TreeTraversal::Iterator::operator++() {
    while (!stack_.empty()) {
        Frame& frame = stack_.back();
        const auto children = nodes_->registry.children(frame.node);

        if (frame.nextChild < children.size()) {
            const NodeId child = children[frame.nextChild++];
            const size_t depth = stack_.size();
            stack_.push_back({child, 0});
            current_ = {child, depth};
            return *this;
        }

        stack_.pop_back();
    }

    current_ = {};
    return *this;
}

TreeTraversal::Iterator TreeTraversal::Iterator::operator++(int) {
    Iterator previous = *this;
    ++*this;
    return previous;
}

TreeTraversal::Iterator TreeTraversal::begin() const {
    return Iterator{nodes_, root_};
}

TreeTraversal TreeView::subtree(NodeId node) const {
    return TreeTraversal{nodes_, node};
}

TreeNodeInfo TreeView::info(NodeId id) const {
    if (!nodes_)
        return {};

    const auto& node = nodes_->registry.require(id);
    TreeNodeInfo result{
        .id = id,
        .parent = node.parent,
        .relation = node.relation,
        .blueprint = node.bp,
        .implementationBlueprint = node.object.bp,
        .kind = node.kind,
        .state = node.state,
        .name = node.name,
        .hasObject = node.object.ptr != nullptr,
        .configured = node.object.configured
    };

    if (node.bp != InvalidBlueprintId)
        result.type = nodes_->blueprints.require(node.bp).shortName();
    if (node.object.bp != InvalidBlueprintId)
        result.implementation = nodes_->blueprints.require(node.object.bp).shortName();

    return result;
}

std::string TreeView::name(NodeId node) const {
    return nodes_ ? nodes_->registry.require(node).name : std::string{};
}

std::string TreeView::label(NodeId id) const {
    if (!nodes_)
        return {};

    const auto& node = nodes_->registry.require(id);
    if (node.bp == InvalidBlueprintId)
        return node.name;

    const std::string_view type = nodes_->blueprints.require(node.bp).shortName();
    return node.name.empty() ? std::string(type) : std::format("{}:{}", type, node.name);
}

std::string TreeView::path(NodeId node) const {
    return nodes_ ? nodes_->ops.stringPath(node) : std::string{};
}

}
