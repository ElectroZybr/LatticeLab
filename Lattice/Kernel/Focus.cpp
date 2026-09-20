#include <Lattice/Kernel/Node.hpp>

namespace Lattice {

RoleId Context::getOrCreateRole(std::string_view name) {
    auto role = roles.find(name);
    if (role == InvalidRoleId)
        role = roles.create(Role{std::string(name)});
    resolvedRoles.resize(roles.size(), InvalidObjectId);
    return role;
}

FocusScopeId Context::createFocusScope(ObjectId owner) {
    objects.require(owner);
    return focusScopes.create(FocusScope{owner}, owner);
}

FocusScopeId Context::parentFocusScope(FocusScopeId scope) const {
    const auto* entry = focusScopes.get(scope);
    if (!entry)
        return InvalidFocusScopeId;
    const auto* owner = objects.get(entry->owner);
    for (const Node* node = owner && owner->node ? owner->node->getParent() : nullptr;
         node; node = node->getParent()) {
        if (node->getFocusScopeId() != InvalidFocusScopeId)
            return node->getFocusScopeId();
    }
    return InvalidFocusScopeId;
}

ObjectId Context::resolveFocus(FocusScopeId origin, RoleId role) const {
    if (origin == InvalidFocusScopeId)
        return role < resolvedRoles.size() ? resolvedRoles[role] : InvalidObjectId;
    while (const auto* scope = focusScopes.get(origin)) {
        for (const auto& entry : scope->roles)
            if (entry.role == role)
                return entry.target;
        origin = parentFocusScope(origin);
    }
    return InvalidObjectId;
}

void Context::resolveActiveRole(RoleId role) {
    resolvedRoles.resize(roles.size(), InvalidObjectId);
    ObjectId target = InvalidObjectId;
    for (auto id : activeChain)
        for (const auto& entry : focusScopes.require(id).roles)
            if (entry.role == role)
                target = entry.target;
    resolvedRoles[role] = target;
}

void Context::setFocus(FocusScopeId scopeId, RoleId role, ObjectId target) {
    focusScopes.require(scopeId);
    roles.require(role);
    if (target != InvalidObjectId)
        objects.require(target);
    auto& entries = focusScopes.get(scopeId)->roles;
    auto entry = std::ranges::find(entries, role, &FocusEntry::role);
    if (entry == entries.end())
        entries.push_back({role, target});
    else
        entry->target = target;
    if (std::ranges::find(activeChain, scopeId) != activeChain.end())
        resolveActiveRole(role);
}

void Context::resetFocus(FocusScopeId scopeId, RoleId role) {
    focusScopes.require(scopeId);
    roles.require(role);
    std::erase_if(focusScopes.get(scopeId)->roles,
                  [role](const auto& entry) { return entry.role == role; });
    if (std::ranges::find(activeChain, scopeId) != activeChain.end())
        resolveActiveRole(role);
}

void Context::rebuildFocus() {
    activeChain.clear();
    for (auto scope = activeScope; scope != InvalidFocusScopeId; scope = parentFocusScope(scope))
        activeChain.push_back(scope);
    std::ranges::reverse(activeChain);
    resolvedRoles.assign(roles.size(), InvalidObjectId);
    for (auto scope : activeChain)
        for (const auto& entry : focusScopes.require(scope).roles)
            resolvedRoles[entry.role] = entry.target;
}

void Context::activateFocus(FocusScopeId scope) {
    focusScopes.require(scope);
    activeScope = scope;
    rebuildFocus();
}

void Context::removeFocusObject(ObjectId object) {
    // Called before ObjectRegistry releases the ID for reuse.
    for (FocusScopeId id = 0; id < focusScopes.size(); ++id) {
        auto* scope = focusScopes.get(id);
        if (!scope)
            continue;
        for (auto& entry : scope->roles) {
            if (entry.target == object) {
                entry.target = InvalidObjectId;
                if (std::ranges::find(activeChain, id) != activeChain.end())
                    resolveActiveRole(entry.role);
            }
        }
    }
    const auto scope = focusScopes.find(object);
    if (scope != InvalidFocusScopeId) {
        if (activeScope == scope)
            activeScope = parentFocusScope(scope);
        focusScopes.destroy(scope);
        rebuildFocus();
    }
}

void* Context::castFocus(ObjectId object, BlueprintId api) const {
    const auto* entry = objects.get(object);
    return entry && entry->node ? entry->node->castObject(api) : nullptr;
}

}
