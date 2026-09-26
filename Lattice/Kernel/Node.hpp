#pragma once

#include <string>

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/Consts.hpp>

namespace Lattice {

struct RuntimeObject {
    void* ptr = nullptr;
    BlueprintId bp = InvalidBlueprintId;
    bool configured = false;
    bool building = false;
};

struct Node {
    std::string name;
    NodeId parent = InvalidNodeId;
    BlueprintId bp = InvalidBlueprintId;
    NodeId relation = InvalidNodeId;
    NodeKind kind = NodeKind::Folder;
    bool exists = true;
    RuntimeObject object;
};

struct NodeKey {
    std::string name;
    NodeId parent = InvalidNodeId;
    BlueprintId type = InvalidBlueprintId;
    uint64_t discriminator = std::numeric_limits<uint64_t>::max();

    bool operator==(const NodeKey&) const = default;
};

struct NodeKeyHash {
    size_t operator()(const NodeKey& key) const noexcept {
        size_t h = std::hash<NodeId>{}(key.parent);
        h ^= std::hash<std::string>{}(key.name) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<BlueprintId>{}(key.type) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint64_t>{}(key.discriminator) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

}
