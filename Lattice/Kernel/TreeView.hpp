#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/Node.hpp>

namespace Lattice {

class NodeSystem;

struct TreeEntry {
    NodeId id = InvalidNodeId;
    size_t depth = 0;
};

struct TreeNodeInfo {
    NodeId id = InvalidNodeId;
    NodeId parent = InvalidNodeId;
    NodeId relation = InvalidNodeId;
    BlueprintId blueprint = InvalidBlueprintId;
    BlueprintId implementationBlueprint = InvalidBlueprintId;
    NodeKind kind = NodeKind::Folder;
    NodeState state = NodeState::Active;
    std::string name;
    std::string type;
    std::string implementation;
    bool hasObject = false;
    bool configured = false;
};

// Lazy pre-order traversal. Structural mutation invalidates active iterators,
// just like mutation of a standard container invalidates its iterators.
class TreeTraversal {
public:
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = TreeEntry;
        using difference_type = std::ptrdiff_t;
        using pointer = const TreeEntry*;
        using reference = const TreeEntry&;

        Iterator() = default;

        reference operator*() const noexcept { return current_; }
        pointer operator->() const noexcept { return &current_; }
        Iterator& operator++();
        Iterator operator++(int);

        friend bool operator==(const Iterator& iterator, std::default_sentinel_t) noexcept {
            return iterator.stack_.empty();
        }

    private:
        friend class TreeTraversal;

        struct Frame {
            NodeId node = InvalidNodeId;
            size_t nextChild = 0;
        };

        Iterator(NodeSystem* nodes, NodeId root);

        NodeSystem* nodes_ = nullptr;
        std::vector<Frame> stack_;
        TreeEntry current_;
    };

    Iterator begin() const;
    std::default_sentinel_t end() const noexcept { return {}; }

private:
    friend class TreeView;

    TreeTraversal(NodeSystem* nodes, NodeId root) : nodes_(nodes), root_(root) {}

    NodeSystem* nodes_ = nullptr;
    NodeId root_ = InvalidNodeId;
};

// Read-only access to the runtime tree. The view deliberately exposes stable
// ids and copied values, never Node references that could be invalidated by a
// later tree mutation.
class TreeView {
public:
    TreeView() = default;
    explicit TreeView(NodeSystem& nodes) : nodes_(&nodes) {}

    explicit operator bool() const noexcept { return nodes_ != nullptr; }

    bool contains(NodeId id) const;
    NodeId root(NodeId from) const;
    NodeId parent(NodeId node) const;
    std::vector<NodeId> children(NodeId node) const;
    TreeTraversal subtree(NodeId node) const;

    TreeNodeInfo info(NodeId node) const;
    std::string name(NodeId node) const;
    std::string label(NodeId node) const;
    std::string path(NodeId node) const;

private:
    NodeSystem* nodes_ = nullptr;
};

}
