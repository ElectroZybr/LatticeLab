#include <Lattice/Kernel/Context.hpp>

#include <format>

#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/Logger.hpp>

namespace Lattice {

ContextId Context::create(std::string_view name) {
    return contexts.create(ContextSlot{
        .name = std::string(name),
        .object = InvalidObjectId,
        .ns = InvalidObjectId
    });
}

ContextId Context::getOrCreate(std::string_view name) {
    const ContextId id = contexts.find(name);

    if (ContextRegistry::valid(id))
        return id;

    return contexts.create(ContextSlot{
        .name = std::string(name),
        .object = InvalidObjectId,
        .ns = InvalidObjectId
    });
}

ObjectId Context::get(ContextId id) const {
    const ContextSlot* slot = contexts.get(id);

    if (!slot)
        return InvalidObjectId;

    return slot->object;
}

ObjectId Context::find(std::string_view name) const {
    const ContextId id = contexts.find(name);

    if (!ContextRegistry::valid(id))
        return InvalidObjectId;

    return get(id);
}

ObjectId Context::namespaceOf(ContextId id) const {
    const ContextSlot* slot = contexts.get(id);

    if (!slot)
        return InvalidObjectId;

    return slot->ns;
}

ObjectId Context::namespaceOf(std::string_view name) const {
    const ContextId id = contexts.find(name);

    if (!ContextRegistry::valid(id))
        return InvalidObjectId;

    return namespaceOf(id);
}

void Context::assign(ContextId id, ObjectId object, ObjectId ns) {
    ContextSlot* slot = contexts.get(id);

    if (!slot)
        return;

    const auto& entry = objects.require(object);

    slot->object = object;

    if (Objects::valid(ns))
        slot->ns = ns;
    else if (entry.node)
        slot->ns = entry.node->nearestNamespaceRoot();

    std::string nsLabel;

    if (Objects::valid(slot->ns)) {
        const auto& nsEntry = objects.require(slot->ns);
        nsLabel = std::format(" <m>[{}]</>", nsEntry.node->stringPath());
    }

    Logger::info(
        "Context",
        "activated slot '{}' ➜ '{}'{}",
        slot->name,
        entry.node->stringPath(),
        nsLabel
    );
}

void Context::activate(ContextId id, ObjectId object) {
    if (!contexts.get(id))
        return;

    assign(id, object);

    Node* node = objects.require(object).node;

    if (node && node->isNamespaceRoot())
        node->applyNamespace();
}

void Context::clear() {
    contexts.clear();
}

void Context::appendTree(Logger::Tree& tree) const {
    for (ContextId id = 0; id < contexts.size(); ++id) {
        const ContextSlot* slot = contexts.get(id);

        if (!slot)
            continue;

        std::string line = std::format("id:{} <c>{}</>", id, slot->name);

        if (!Objects::valid(slot->object)) {
            line += " <gr>➜ inactive</>";
        } else {
            const auto& entry = objects.require(slot->object);
            line += std::format(" <gr>➜ {}</>", entry.node->stringPath());
        }

        if (Objects::valid(slot->ns)) {
            const auto& nsEntry = objects.require(slot->ns);
            line += std::format(" <m>[{}]</>", nsEntry.node->stringPath());
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