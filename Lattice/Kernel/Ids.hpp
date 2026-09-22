#pragma once
#include <cstdint>
#include <limits>
#include <string_view>

namespace Lattice {

enum class NodeKind : uint8_t {
    Folder,
    Component,
    Slot,
    Binding,
    Mount
};

using NodeId = uint32_t;
using BlueprintId = uint32_t;
using CapabilityId = uint32_t;

inline constexpr NodeId InvalidNodeId = std::numeric_limits<NodeId>::max();
inline constexpr BlueprintId InvalidBlueprintId = std::numeric_limits<BlueprintId>::max();
inline constexpr CapabilityId InvalidCapabilityId = std::numeric_limits<CapabilityId>::max();

inline constexpr std::string_view DefaultInstanceName{};

}