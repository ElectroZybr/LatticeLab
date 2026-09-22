#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include "Lattice/Kernel/Node.hpp"

namespace Lattice {

NodeId NodeFactory::createNode(NodeId parent, std::string_view name, BlueprintId bp, NodeKind kind) {
    Node node{
        .name = std::string(name),
        .parent = parent,
        .bp = bp,
        .kind = kind
    };

    return nodes_.create(std::move(node));
}

NodeId NodeFactory::folder(NodeId parent, std::string_view name) {
    return createNode(parent, name, InvalidBlueprintId, NodeKind::Folder);
}

NodeId NodeFactory::slot(NodeId parent, BlueprintId api, std::string_view instance) {
    blueprints_.require(api);

    if (const NodeId existing = nodes_.find(instance, parent); existing != InvalidNodeId) {
        const auto& node = nodes_.require(existing);

        if (node.kind == NodeKind::Slot && node.bp == api)
            return existing;
    }

    return createNode(parent, instance, api, NodeKind::Slot);
}

NodeId NodeFactory::component(NodeId parent, BlueprintId api, std::string_view instance, const void* desc) {
    blueprints_.require(api);

    const BlueprintId impl = blueprints_.resolveImplementation(api);

    if (const NodeId existing = nodes_.find(instance, parent); existing != InvalidNodeId) {
        const auto& node = nodes_.require(existing);

        if (node.kind == NodeKind::Component && node.bp == impl)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", instance, parent);
    }

    const NodeId id = createNode(parent, instance, impl, NodeKind::Component);
    auto& node = nodes_.require(id);
    const auto& blueprint = blueprints_.require(impl);

    try {
        if (!blueprint.meta.create)
            throw Exception("NodeFactory", "Blueprint '{}' has no create callback", blueprint.name);

        node.object.ptr = blueprint.meta.create(NodeBuildView{id, *this, blueprints_}, desc);
        node.object.bp = impl;

        if (!node.object.ptr)
            throw Exception("NodeFactory", "Blueprint '{}' returned null", blueprint.name);

    } catch (...) {
        if (node.object.ptr && blueprint.meta.destroy)
            blueprint.meta.destroy(node.object.ptr);

        nodes_.destroy(id);
        throw;
    }

    return id;
}

NodeId NodeFactory::component(NodeId parent, std::string_view blueprint, std::string_view instance, const void* desc) {
    const BlueprintId id = blueprints_.find(blueprint);
    if (id == InvalidBlueprintId)
        throw Exception("NodeFactory", "Unknown blueprint '{}'", blueprint);
    return component(parent, id, instance, desc);
}

NodeId NodeFactory::slot(NodeId parent, std::string_view api, std::string_view instance) {
    const BlueprintId id = blueprints_.find(api);
    if (id == InvalidBlueprintId)
        throw Exception("NodeFactory", "Unknown API '{}'", api);
    return slot(parent, id, instance);
}

NodeId NodeFactory::binding(NodeId parent, std::string_view name) {
    if (const NodeId existing = nodes_.find(name, parent); existing != InvalidNodeId) {
        const auto& node = nodes_.require(existing);

        if (node.kind == NodeKind::Binding)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", name, parent);
    }

    return createNode(parent, name, InvalidBlueprintId, NodeKind::Binding);
}

NodeId NodeFactory::mount(NodeId parent, BlueprintId blueprint, std::string_view instance) {
    blueprints_.require(blueprint);

    if (const NodeId existing = nodes_.find(instance, parent); existing != InvalidNodeId) {
        const auto& node = nodes_.require(existing);

        if (node.kind == NodeKind::Mount && node.bp == blueprint)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", instance, parent);
    }

    return createNode(parent, instance, blueprint, NodeKind::Mount);
}

}