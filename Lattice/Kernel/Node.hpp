#pragma once

#include <string>

#include <Lattice/Kernel/Blueprints.hpp>
#include "Lattice/Kernel/Ids.hpp"

namespace Lattice {

struct RuntimeObject {
    void* ptr = nullptr;
    BlueprintId bp = InvalidBlueprintId;
    bool configured = false;
};

struct Node {
    std::string name;
    NodeId parent = InvalidNodeId;
    BlueprintId bp = InvalidBlueprintId;
    NodeKind kind = NodeKind::Folder;
    RuntimeObject object;
    CapabilityId caps = InvalidCapabilityId;
    bool exists = true;
};

struct NodeKey {
    std::string name;
    NodeId parent = InvalidNodeId;

    bool operator==(const NodeKey&) const = default;
};

struct NodeKeyHash {
    size_t operator()(const NodeKey& key) const noexcept {
        size_t h = std::hash<NodeId>{}(key.parent);
        h ^= std::hash<std::string>{}(key.name) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

}