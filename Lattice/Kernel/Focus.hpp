// #pragma once

// #include <cstdint>
// #include <limits>
// #include <string>
// #include <vector>
// #include <Lattice/Kernel/Objects.hpp>
// #include <Lattice/Kernel/Blueprints.hpp>
// #include <Lattice/Kernel/ObjectRegistry.hpp>

// namespace Lattice {

// class Context;
// class Node;
// using RoleId = uint32_t;
// using FocusScopeId = uint32_t;
// inline constexpr RoleId InvalidRoleId = std::numeric_limits<RoleId>::max();
// inline constexpr FocusScopeId InvalidFocusScopeId = std::numeric_limits<FocusScopeId>::max();

// struct Role {
//     std::string name;
//     bool exists = true;
// };
// struct FocusEntry {
//     RoleId role;
//     ObjectId target;
// };
// struct FocusScope {
//     ObjectId owner = InvalidObjectId;
//     BlueprintId type = Blueprints::InvalidId;
//     std::vector<FocusEntry> roles;
//     bool exists = true;
// };
// using RoleRegistry = ObjectRegistry<Role, RoleId, std::string>;
// using FocusScopeRegistry = ObjectRegistry<FocusScope, FocusScopeId, ObjectId>;

// template<class T>
// class Focus {
// public:
//     Focus() = default;

//     ObjectId id() const;
//     T* get() const;

//     T* operator->() const { return get(); }
//     T& operator*() const { return *get(); }

//     explicit operator bool() const { return get() != nullptr; }

// private:
//     friend class Context;
//     friend class Node;
//     Focus(Context& context, RoleId role, FocusScopeId origin)
//         : context_(&context), role_(role), originScope_(origin) {}
//     Context* context_ = nullptr;
//     RoleId role_ = InvalidRoleId;
//     // Invalid origin identifies a global handle.
//     FocusScopeId originScope_ = InvalidFocusScopeId;
// };

// }
