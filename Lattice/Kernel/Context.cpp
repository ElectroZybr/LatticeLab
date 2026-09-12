#include <Lattice/Kernel/Context.hpp>

#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Kernel/Node.hpp>


namespace Lattice {

SlotId Context::addSlot(std::string_view name) {
    ctx_slots.push_back({std::string(name), nullptr});
    return static_cast<SlotId>(ctx_slots.size() - 1);
}

SlotId Context::getSlot(std::string_view name) {
    for (SlotId i = 0; i < ctx_slots.size(); ++i)
        if (ctx_slots[i].name == name)
            return i;

    return addSlot(name);
}

Binding* Context::get(SlotId id) {
    if (id >= ctx_slots.size())
        return nullptr;

    return ctx_slots[id].binding;
}

const Binding* Context::get(SlotId id) const {
    if (id >= ctx_slots.size())
        return nullptr;

    return ctx_slots[id].binding;
}

void Context::activate(SlotId slot, ObjectId id) {
    auto& ctx_slot = ctx_slots[slot];
    auto& binding = bindings.get(id);

    if (objects[id].name != ctx_slot.name)
        throw Exception("Context", "binding '{}' cannot be activated in slot '{}'", objects[id].name, ctx_slot.name);

    ctx_slot.binding = &binding;
    Logger::info("Context", "activated slot '{}' ➜ '{}'", ctx_slot.name, objects[id].node->stringPath());
}

void Context::invoke(SlotId id) {
    auto* binding = get(id);

    if (!binding || !binding->invoke)
        return;

    binding->invoke(Value{});
}
}