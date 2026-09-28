#pragma once

#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>

namespace Lattice {

class NodeRegistry : public ObjectRegistry<Node, NodeId, NodeKey, NodeKeyHash> {
    using Base = ObjectRegistry<Node, NodeId, NodeKey, NodeKeyHash>;
    std::vector<std::vector<NodeId>> children_;
    std::unordered_map<NodeId, std::vector<NodeId>> references_;
public:
    void clear() {
        children_.clear();
        references_.clear();
        Base::clear();
    }
    
    NodeId create(Node node, uint64_t discriminator = std::numeric_limits<uint64_t>::max()) {
        if (node.parent != InvalidNodeId)
            require(node.parent);

        NodeKey key{node.name, node.parent, node.bp, discriminator};
        const NodeId id = Base::create(std::move(node), std::move(key));

        if (id >= children_.size())
            children_.resize(id + 1);

        const NodeId parent = require(id).parent;
        if (parent != InvalidNodeId)
            children_[parent].push_back(id);

        children_[id].clear();

        return id;
    }

    NodeId find(
        std::string_view name,
        NodeId parent = InvalidNodeId,
        BlueprintId type = InvalidBlueprintId,
        uint64_t discriminator = std::numeric_limits<uint64_t>::max()
    ) const {
        return Base::find(NodeKey{std::string(name), parent, type, discriminator});
    }

    std::span<const NodeId> children(NodeId id) const {
        require(id);
        return children_[id];
    }

    void link(NodeId reference, NodeId target) {
        auto& node = require(reference);
        require(target);

        if (node.relation == target)
            return;

        unlink(reference);
        node.relation = target;
        references_[target].push_back(reference);
    }

    void unlink(NodeId reference) {
        auto& node = require(reference);
        const NodeId target = node.relation;
        if (target == InvalidNodeId)
            return;

        if (auto found = references_.find(target); found != references_.end()) {
            std::erase(found->second, reference);
            if (found->second.empty())
                references_.erase(found);
        }

        node.relation = InvalidNodeId;
    }

    std::span<const NodeId> references(NodeId target) const {
        require(target);
        const auto found = references_.find(target);
        return found == references_.end() ? std::span<const NodeId>{} : found->second;
    }

    void destroy(NodeId id) {
        const NodeId parent = require(id).parent;

        const auto childIds = std::vector<NodeId>(children(id).begin(), children(id).end());
        for (NodeId child : childIds)
            destroy(child);

        children_[id].clear();

        if (parent != InvalidNodeId) {
            auto& siblings = children_[parent];
            std::erase(siblings, id);
        }

        unlink(id);

        if (auto found = references_.find(id); found != references_.end()) {
            for (NodeId reference : found->second)
                if (auto* node = get(reference); node && node->relation == id)
                    node->relation = InvalidNodeId;
            references_.erase(found);
        }

        Base::destroy(id);
    }
};

}
