#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>

namespace Lattice {

RoleId NodeContext::role(std::string_view name) {
    RoleId id = roles_.find(std::string(name));
    if (id == InvalidRoleId)
        id = roles_.create(Role{std::string(name)}, std::string(name));
    return id;
}

RoleId NodeContext::findRole(std::string_view name) const {
    return roles_.find(std::string(name));
}

ContextScopeId NodeContext::createScope(NodeId owner, BlueprintId type) {
    if (const auto id = scopes_.find(owner); id != InvalidContextScopeId)
        return id;

    const ContextScopeId id = scopes_.create(ContextScope{
        .owner = owner,
        .type = type
    }, owner);

    if (registry_.require(owner).parent == InvalidNodeId)
        rootScope_ = id;

    return id;
}

ContextScopeId NodeContext::findScope(NodeId owner) const {
    return scopes_.find(owner);
}

bool NodeContext::isActive(ContextScopeId scope) const {
    return std::ranges::find(activeScopes_, scope) != activeScopes_.end();
}

void NodeContext::set(ContextScopeId scopeId, RoleId roleId, NodeId target) {
    auto& scope = scopes_.require(scopeId);
    roles_.require(roleId);

    for (auto& entry : scope.roles) {
        if (entry.role == roleId) {
            entry.target = target;
            return;
        }
    }

    scope.roles.push_back({roleId, target});
}

void NodeContext::reset(ContextScopeId scopeId, RoleId roleId) {
    auto& scope = scopes_.require(scopeId);
    std::erase_if(scope.roles, [roleId](const auto& entry) {
        return entry.role == roleId;
    });
}

NodeId NodeContext::get(ContextScopeId scopeId, RoleId roleId) const {
    const auto& scope = scopes_.require(scopeId);

    for (const auto& entry : scope.roles)
        if (entry.role == roleId)
            return entry.target;

    return InvalidNodeId;
}

ContextScopeId NodeContext::parentScope(ContextScopeId scope) const {
    NodeId owner = scopes_.require(scope).owner;

    for (NodeId parent = registry_.require(owner).parent; parent != InvalidNodeId; parent = registry_.require(parent).parent) {
        const ContextScopeId found = findScope(parent);
        if (found != InvalidContextScopeId) return found;
    }

    return InvalidContextScopeId;
}

std::optional<NodeId> NodeContext::lookup(ContextScopeId scopeId, RoleId roleId) const {
    const auto& scope = scopes_.require(scopeId);

    for (const auto& entry : scope.roles)
        if (entry.role == roleId)
            return entry.target;

    return {};
}

NodeId NodeContext::resolve(ContextScopeId scope, RoleId role) const {
    while (scope != InvalidContextScopeId) {
        if (auto value = lookup(scope, role))
            return *value;

        scope = parentScope(scope);
    }

    return InvalidNodeId;
}

NodeId NodeContext::resolve(RoleId roleId) const {
    NodeId result = InvalidNodeId;

    if (rootScope_ != InvalidContextScopeId)
        if (auto value = lookup(rootScope_, roleId))
            result = *value;

    for (ContextScopeId active : activeScopes_) {
        std::vector<ContextScopeId> chain;

        for (ContextScopeId scope = active; scope != InvalidContextScopeId; scope = parentScope(scope))
            chain.push_back(scope);

        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            if (auto value = lookup(*it, roleId))
                result = *value;
    }

    return result;
}

ContextScopeId NodeContext::active(BlueprintId type) const {
    for (ContextScopeId id : activeScopes_)
        if (scopes_.require(id).type == type)
            return id;

    return InvalidContextScopeId;
}

void NodeContext::activate(ContextScopeId scope) {
    scopes_.require(scope);

    if (scope == rootScope_)
        return;

    if (std::ranges::find(activeScopes_, scope) == activeScopes_.end())
        activeScopes_.push_back(scope);
}

void NodeContext::deactivate(ContextScopeId scope) {
    std::erase(activeScopes_, scope);
}

ContextScopeId NodeContext::nearestScope(NodeId owner) const {
    for (NodeId id = owner; id != InvalidNodeId; id = registry_.require(id).parent) {
        const ContextScopeId scope = findScope(id);
        if (scope != InvalidContextScopeId)
            return scope;
    }

    return InvalidContextScopeId;
}

void NodeContext::destroyScope(ContextScopeId scope) {
    scopes_.require(scope);

    std::erase(activeScopes_, scope);

    if (rootScope_ == scope)
        rootScope_ = InvalidContextScopeId;

    scopes_.destroy(scope);
}

void NodeContext::removeTarget(NodeId target) {
    for (ContextScopeId id = 0; id < scopes_.size(); ++id) {
        auto* scope = scopes_.get(id);
        if (!scope) continue;

        std::erase_if(scope->roles, [target](const ContextEntry& entry) {
            return entry.target == target;
        });
    }
}

}