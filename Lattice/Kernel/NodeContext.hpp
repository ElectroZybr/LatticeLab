#pragma once

#include <string>
#include <vector>

#include "Lattice/Kernel/Consts.hpp"
#include "Lattice/Kernel/ObjectRegistry.hpp"


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
};

struct ContextScope {
    NodeId owner = InvalidNodeId;
    BlueprintId type = InvalidBlueprintId;
    std::vector<ContextEntry> roles;
    bool exists = true;
};

using ContextRegistry = ObjectRegistry<ContextScope, ContextScopeId, NodeId>;

class NodeContext {
    NodeRegistry& nodes_;
    
    RoleRegistry roles_;
    ContextRegistry scopes_;
    ContextScopeId rootScope_ = InvalidContextScopeId;
    std::vector<ContextScopeId> activeScopes_;

public:
    explicit NodeContext(NodeRegistry& nodes) : nodes_(nodes) {}

    RoleId role(std::string_view name);
    RoleId findRole(std::string_view name) const;

    ContextScopeId createScope(NodeId owner, BlueprintId type = InvalidBlueprintId);
    ContextScopeId parentScope(ContextScopeId scope) const;
    ContextScopeId findScope(NodeId owner) const;
    ContextScopeId active(BlueprintId type) const;
    ContextScopeId nearestScope(NodeId owner) const;
    
    void activate(ContextScopeId scope);
    void deactivate(ContextScopeId scope);
    bool isActive(ContextScopeId scope) const;

    void set(ContextScopeId scope, RoleId role, NodeId target);
    void reset(ContextScopeId scope, RoleId role);
    void destroyScope(ContextScopeId scope);
    void removeTarget(NodeId target);
    NodeId get(ContextScopeId scope, RoleId role) const;
    std::optional<NodeId> lookup(ContextScopeId scopeId, RoleId roleId) const;
    NodeId resolve(ContextScopeId scope, RoleId role) const;
    NodeId resolve(RoleId roleId) const;

    ContextScopeId root() const noexcept { return rootScope_; }
    size_t scopeCount() const noexcept { return scopes_.size(); }
    const ContextScope* scope(ContextScopeId id) const { return scopes_.get(id); }
    std::span<const ContextScopeId> activeScopes() const noexcept { return activeScopes_; }
    std::string_view roleName(RoleId id) const { return roles_.require(id).name; }
    size_t roleCount() const noexcept { return roles_.size(); }
    bool hasRole(RoleId id) const { return roles_.get(id) != nullptr; }
};

}