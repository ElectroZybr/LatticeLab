#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Bindings.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Tools/LogTree.hpp>

namespace Lattice {

class Node;

inline constexpr std::string_view DefaultBlueprintsPath = "Blueprints";
inline constexpr std::string_view DefaultInstanceName = "default";

struct Meta {
    void* (*create)(Node&) = nullptr;
    void (*destroy)(Node&) = nullptr;
    void (*configure)(Node&) = nullptr;
};

using ContextId = uint32_t;
inline constexpr ContextId InvalidContextId = std::numeric_limits<ContextId>::max();

struct ContextSlot {
    std::string name;
    ObjectId object = InvalidObjectId;
    ObjectId ns = InvalidObjectId;
    bool exists = true;
};

using ContextRegistry = ObjectRegistry<ContextSlot, ContextId, std::string>;

class Context {
public:
    ContextId create(std::string_view name);
    ContextId getOrCreate(std::string_view name);
    ObjectId get(ContextId id) const;
    ObjectId find(std::string_view name) const;

    ObjectId namespaceOf(ContextId id) const;
    ObjectId namespaceOf(std::string_view name) const;

    // Ставит слот. Если object является корнем неймспейса, переключает и его экспорт.
    void activate(ContextId id, ObjectId object);

    // Ставит слот без переключения неймспейса.
    void assign(ContextId id, ObjectId object, ObjectId ns = InvalidObjectId);

    void clear();

    void printTree() const;

    Bindings bindings;
    Objects objects;
    std::vector<std::unique_ptr<Meta>> metas;
    ContextRegistry contexts;
    
private:
    void appendTree(Logger::Tree& tree) const;
};

}