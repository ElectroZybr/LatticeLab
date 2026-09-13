#include <Lattice/Kernel/Context.hpp>

#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Kernel/Node.hpp>


namespace Lattice {

SlotId Context::addSlot(std::string_view name) {
    ctx_slots.push_back({std::string(name), InvalidObjectId});
    return static_cast<SlotId>(ctx_slots.size() - 1);
}

SlotId Context::getSlot(std::string_view name) {
    for (SlotId i = 0; i < ctx_slots.size(); ++i)
        if (ctx_slots[i].name == name)
            return i;

    return addSlot(name);
}

ObjectId Context::get(SlotId id) {
    if (id >= ctx_slots.size())
        return InvalidObjectId;

    return ctx_slots[id].object;
}

ObjectId Context::active(std::string_view name) const {
    for (const auto& slot : ctx_slots) {
        if (slot.name == name)
            return slot.object;
    }

    return InvalidObjectId;
}

void Context::activate(SlotId slot, ObjectId object) {
    if (slot >= ctx_slots.size())
        return;

    auto& ctx_slot = ctx_slots[slot];
    const auto& entry = objects.require(object);

    // if (entry.name != ctx_slot.name)
    //     throw Exception("Context", "binding '{}' cannot be activated in slot '{}'", objects[object].name, ctx_slot.name);

    ctx_slot.object = object;
    Logger::info("Context", "activated slot '{}' ➜ '{}'", ctx_slot.name, objects[object].node->stringPath());
}

void Context::appendTree(Logger::Tree& tree) const {
    for (int i = 0; i < ctx_slots.size(); ++i) {
        const auto& slot = ctx_slots[i];
        std::string line = std::format("id:{} <c>{}</>", i, slot.name);

        if (!Objects::valid(slot.object)) {
            line += " <gr>➜ inactive</>";
        } else {
            const auto& entry = objects.require(slot.object);
            line += std::format(" <gr>➜ {}</>", entry.node->stringPath());
        }

        tree.node(line, 0);
    }
}

void Context::printTree() const {
    Logger::Tree tree("Context");
    appendTree(tree);
    tree.print();
}
}