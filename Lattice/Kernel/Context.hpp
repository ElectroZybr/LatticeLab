#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <Lattice/Kernel/Focus.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/Bindings.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Tools/LogTree.hpp>

namespace Lattice {

class Node;

inline constexpr std::string_view DefaultInstanceName = "";

using ComponentsRegistry = ObjectRegistry<Object, ObjectId, ObjectKey, ObjectKeyHash>;


class Context {
public:
    RoleId getOrCreateRole(std::string_view name);
    void setFocus(FocusScopeId scope, RoleId role, ObjectId target);
    void resetFocus(FocusScopeId scope, RoleId role);
    void activateFocus(FocusScopeId scope);
    ObjectId resolveFocus(FocusScopeId origin, RoleId role) const;

    template<class T>
    Focus<T> focus(std::string_view role = typeKey<T>()) {
        static_assert(!std::is_same_v<T, Node>, "Use id() for low-level node access");
        return Focus<T>(*this, getOrCreateRole(role), InvalidFocusScopeId);
    }

    RoleRegistry roles;
    FocusScopeRegistry focusScopes;
    FocusScopeId activeScope = InvalidFocusScopeId;
    std::vector<FocusScopeId> activeChain;
    std::vector<ObjectId> resolvedRoles;

    void printTree() const;

    Bindings bindings;
    Blueprints blueprints;
    ComponentsRegistry objects;
    
private:
    friend class Node;
    template<class T> friend class Focus;
    FocusScopeId createFocusScope(ObjectId owner);
    FocusScopeId parentFocusScope(FocusScopeId scope) const;
    void removeFocusObject(ObjectId object);
    void rebuildFocus();
    void resolveActiveRole(RoleId role);
    void* castFocus(ObjectId object, BlueprintId api) const;
    void appendTree(Logger::Tree& tree) const;
};

template<class T>
ObjectId Focus<T>::id() const {
    return context_ ? context_->resolveFocus(originScope_, role_) : InvalidObjectId;
}

template<class T>
T* Focus<T>::get() const {
    return context_ ? static_cast<T*>(context_->castFocus(
        id(), context_->blueprints.find(typeKey<T>()))) : nullptr;
}

}
