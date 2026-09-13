#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Bindings.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include "Lattice/Tools/LogTree.hpp"


namespace Lattice {

class Node;

inline constexpr std::string_view DefaultBlueprintsPath = "Blueprints";
inline constexpr std::string_view DefaultInstanceName = "default";

struct Meta {
    void* (*create)(Node&) = nullptr;
    void (*destroy)(Node&) = nullptr;
    void (*configure)(Node&) = nullptr;
};

using SlotId = uint32_t;

struct ContextSlot {
    std::string name;
    ObjectId object = InvalidObjectId;
};

class Context {

public:
    SlotId addSlot(std::string_view name);

    SlotId getSlot(std::string_view name);

    ObjectId get(SlotId id);

    ObjectId active(std::string_view name) const;

    void activate(SlotId slot, ObjectId id);

    void clear() { ctx_slots.clear(); }

    void printTree() const;

    Bindings bindings;
    Objects objects;
    std::vector<std::unique_ptr<Meta>> metas;

private:
    std::vector<ContextSlot> ctx_slots;

    void appendTree(Logger::Tree& tree) const;
};

}