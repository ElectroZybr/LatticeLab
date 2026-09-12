#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Bindings.hpp>
#include <Lattice/Kernel/Objects.hpp>


namespace Lattice {

class Node;

inline constexpr std::string_view DefaultBlueprintsPath = "Blueprints";
inline constexpr std::string_view DefaultInstanceName = "default";

struct Meta {
    void* (*create)(Node&) = nullptr;
    void (*destroy)(Node&) = nullptr;
    void (*configure)(Node&) = nullptr;
};

struct Primitives {
    ObjectId action;
    ObjectId param;
};

using SlotId = uint32_t;

struct ContextSlot {
    std::string name;
    Binding* binding = nullptr;
};

class Context {

public:
    SlotId addSlot(std::string_view name);

    SlotId getSlot(std::string_view name);

    Binding* get(SlotId id);

    const Binding* get(SlotId id) const;

    void activate(SlotId slot, ObjectId id);

    void invoke(SlotId id);

    template<typename T>
    T getValue(SlotId id) const {
        const auto binding = get(id);

        if (!binding || !binding->get)
            throw Exception("Context", "binding '{}' is not readable", ctx_slots[id].name);

        return binding->get(binding->object).get<T>();
    }

    template<typename T>
    void set(SlotId id, T value) {
        auto binding = get(id);

        if (!binding || !binding->set)
            throw Exception("Context", "binding '{}' is not writable", ctx_slots[id].name);

        binding->set(binding, Value{std::move(value)});
    }

    void clear() { ctx_slots.clear(); }

    Bindings bindings;
    Objects objects;
    Primitives primitives;
    std::vector<std::unique_ptr<Meta>> metas;

private:
    std::vector<ContextSlot> ctx_slots;
};

}