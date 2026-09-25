#pragma once
#include <cstdint>
#include <limits>
#include <string_view>

namespace Lattice {

struct Component {};
struct SubsystemAPI : public Component {};

enum class NodeKind : uint8_t {
    Folder,
    Component,
    Slot,
    Binding,
    Mount,
    SharedMount
};

using NodeId = uint32_t;
using RoleId = uint32_t;
using BlueprintId = uint32_t;
using ContextScopeId = uint32_t;

inline constexpr NodeId InvalidNodeId = std::numeric_limits<NodeId>::max();
inline constexpr RoleId InvalidRoleId = std::numeric_limits<RoleId>::max();
inline constexpr BlueprintId InvalidBlueprintId = std::numeric_limits<BlueprintId>::max();
inline constexpr ContextScopeId InvalidContextScopeId = std::numeric_limits<ContextScopeId>::max();

inline constexpr std::string_view DefaultInstanceName{};

}
