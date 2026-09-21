#include <Lattice/Kernel/Node.hpp>

namespace Lattice {

RoleId Context::getOrCreateRole(std::string_view name) {
    auto role = roles.find(name);
    if (role == InvalidRoleId)
        role = roles.create(Role{std::string(name)});
    resolvedRoles.resize(roles.size(), InvalidObjectId);
    return role;
}

RoleId Context::findRole(std::string_view name) const {
    const auto role = roles.find(name);
    if (role != InvalidRoleId)
        return role;
    const auto blueprint = blueprints.resolve(name);
    if (blueprint == Blueprints::InvalidId)
        return InvalidRoleId;
    return roles.find(blueprints.require(blueprint).name);
}

FocusScopeId Context::createFocusScope(ObjectId owner, BlueprintId type) {
    objects.require(owner);
    return focusScopes.create(FocusScope{owner, type}, owner);
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

void Context::overlayActive(RoleId role) {
    const bool all = role == InvalidRoleId;
    std::vector<FocusScopeId> overlay;
    if (focusScopes.get(rootScope))
        overlay.push_back(rootScope);
    for (auto selected : activeScopes) {
        std::vector<FocusScopeId> chain;
        for (auto scope = selected; scope != InvalidFocusScopeId; scope = parentFocusScope(scope))
            chain.push_back(scope);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            if (std::ranges::find(overlay, *it) == overlay.end())
                overlay.push_back(*it);
    }
    for (auto id : overlay)
        for (const auto& entry : focusScopes.require(id).roles)
            if (all || entry.role == role)
                resolvedRoles[entry.role] = entry.target;
}

void Context::resolveActiveRole(RoleId role) {
    resolvedRoles.resize(roles.size(), InvalidObjectId);
    resolvedRoles[role] = InvalidObjectId;
    overlayActive(role);
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
    resolveActiveRole(role);
}

void Context::resetFocus(FocusScopeId scopeId, RoleId role) {
    focusScopes.require(scopeId);
    roles.require(role);
    std::erase_if(focusScopes.get(scopeId)->roles,
                  [role](const auto& entry) { return entry.role == role; });
    resolveActiveRole(role);
}

FocusScopeId Context::activeFocus(BlueprintId type) const {
    for (auto scope : activeScopes)
        if (focusScopes.require(scope).type == type)
            return scope;
    return InvalidFocusScopeId;
}

void Context::activateFocusIfTyped(FocusScopeId scope) {
    const auto* requested = focusScopes.get(scope);
    if (!requested || scope == rootScope || requested->type == Blueprints::InvalidId)
        return;
    std::vector<BlueprintId> seen;
    for (auto id = scope; id != rootScope && id != InvalidFocusScopeId; id = parentFocusScope(id)) {
        const auto type = focusScopes.require(id).type;
        if (type == Blueprints::InvalidId)
            continue;
        if (std::ranges::find(seen, type) != seen.end())
            return;
        seen.push_back(type);
    }
    activateFocus(scope);
}

void Context::rebuildFocus() {
    resolvedRoles.assign(roles.size(), InvalidObjectId);
    overlayActive();
}

void Context::activateFocus(FocusScopeId scope) {
    const auto& requested = focusScopes.require(scope);
    if (scope == rootScope)
        return; // Root is the floor, not a selection.
    if (requested.type == Blueprints::InvalidId)
        throw Exception("Focus", "Scope requires an explicit type for activation");

    std::vector<FocusScopeId> chain;
    for (auto id = scope; id != rootScope && id != InvalidFocusScopeId; id = parentFocusScope(id)) {
        const auto type = focusScopes.require(id).type;
        if (type != Blueprints::InvalidId) {
            for (auto child : chain)
                if (focusScopes.require(child).type == type)
                    throw Exception("Focus", "A focus chain cannot activate two scopes of the same type");
            chain.push_back(id);
        }
    }
    // Validate the whole chain before changing any selection.
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        const auto candidate = *it;
        const auto type = focusScopes.require(candidate).type;
        const auto previous = activeFocus(type);
        if (previous != InvalidFocusScopeId && previous != candidate) {
            const auto owner = focusScopes.require(previous).owner;
            std::erase_if(activeScopes, [&](auto active) {
                return objects.require(focusScopes.require(active).owner).node->isUnder(owner);
            });
        }
        std::vector<FocusScopeId> descendants;
        if (candidate == scope) {
            const auto owner = focusScopes.require(candidate).owner;
            std::erase_if(activeScopes, [&](auto active) {
                if (!objects.require(focusScopes.require(active).owner).node->isUnder(owner))
                    return false;
                if (active != candidate)
                    descendants.push_back(active);
                return true;
            });
        }
        if (std::ranges::find(activeScopes, candidate) == activeScopes.end())
            activeScopes.push_back(candidate);
        activeScopes.insert(activeScopes.end(), descendants.begin(), descendants.end());
    }
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
                resolveActiveRole(entry.role);
            }
        }
    }
    const auto scope = focusScopes.find(object);
    if (scope != InvalidFocusScopeId) {
        std::erase(activeScopes, scope);
        if (rootScope == scope)
            rootScope = InvalidFocusScopeId;
        focusScopes.destroy(scope);
        rebuildFocus();
    }
}

void* Context::castFocus(ObjectId object, BlueprintId api) const {
    const auto* entry = objects.get(object);
    return entry && entry->node ? entry->node->castObject(api) : nullptr;
}

}
