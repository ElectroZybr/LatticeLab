#pragma once

#include <span>
#include <string>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>


namespace Lattice {

class NodeRegistry;

struct Role {
    std::string name;
    bool exists = true;
};

using RoleRegistry = ObjectRegistry<Role, RoleId, std::string>;

struct ContextEntry {
    RoleId role = InvalidRoleId;
    NodeId target = InvalidNodeId;
    std::vector<NodeId> candidates;
};

struct ContextScope {
    NodeId owner = InvalidNodeId;
    BlueprintId type = InvalidBlueprintId;
    std::vector<ContextEntry> roles;
    bool exists = true;
};

using ContextRegistry = ObjectRegistry<ContextScope, ContextScopeId, NodeId>;

enum class ContextResolutionState : uint8_t {
    Missing,
    Resolved,
    Ambiguous
};

struct ContextResolution {
    ContextResolutionState state = ContextResolutionState::Missing;
    NodeId target = InvalidNodeId;
    std::span<const NodeId> candidates;

    explicit operator bool() const noexcept {
        return state == ContextResolutionState::Resolved;
    }
};

class NodeContext {
    NodeRegistry& registry_;

    RoleRegistry roles_;
    ContextRegistry scopes_;
    ContextScopeId rootScope_ = InvalidContextScopeId;
    std::vector<ContextScopeId> activeScopes_;

public:
    explicit NodeContext(NodeRegistry& registry) : registry_(registry) {}

    // roles
    RoleId role(std::string_view name);
    RoleId findRole(std::string_view name) const;
    std::string_view roleName(RoleId id) const;

    // scopes
    ContextScopeId createScope(NodeId owner, BlueprintId type = InvalidBlueprintId);
    ContextScopeId parentScope(ContextScopeId scope) const;
    ContextScopeId findScope(NodeId owner) const;
    ContextScopeId nearestScope(NodeId owner) const;
    ContextScopeId active(BlueprintId type) const;

    void activate(ContextScopeId scope);
    void deactivate(ContextScopeId scope);
    bool isActive(ContextScopeId scope) const;
    void destroyScope(ContextScopeId scope);

    // candidates
    void addCandidate(ContextScopeId scope, RoleId role, NodeId target);
    void removeCandidate(ContextScopeId scope, RoleId role, NodeId target);
    void removeTarget(NodeId target);

    // explicit choice / focus
    void set(ContextScopeId scope, RoleId role, NodeId target);
    void reset(ContextScopeId scope, RoleId role);

    // resolution
    // candidates is a non-owning view invalidated by context mutation.
    ContextResolution resolveInfo(ContextScopeId scope, RoleId role) const;
    ContextResolution resolveInfo(RoleId role) const;

    NodeId resolve(ContextScopeId scope, RoleId role) const;
    NodeId resolve(RoleId role) const;

    // helpers
    ContextScopeId root() const noexcept { return rootScope_; }
    size_t scopeCount() const noexcept { return scopes_.size(); }
    const ContextScope* scope(ContextScopeId id) const { return scopes_.get(id); }
    std::span<const ContextScopeId> activeScopes() const noexcept { return activeScopes_; }
    size_t roleCount() const noexcept { return roles_.size(); }
    bool hasRole(RoleId id) const { return roles_.get(id) != nullptr; }

private:
    ContextEntry* findEntry(ContextScopeId scope, RoleId role);
    const ContextEntry* findEntry(ContextScopeId scope, RoleId role) const;
    ContextEntry& requireEntry(ContextScopeId scope, RoleId role);
};

}
