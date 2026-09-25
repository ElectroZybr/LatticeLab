#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/NodeOps.hpp>

namespace Lattice {

NodeId NodeFactory::createNode(NodeId parent, std::string_view name, BlueprintId bp, NodeKind kind) {
    NodeId previous = InvalidNodeId;

    if (parent != InvalidNodeId && bp != InvalidBlueprintId) {
        for (NodeId child : nodeSystem_.registry.children(parent)) {
            if (nodeSystem_.registry.require(child).bp == bp) {
                previous = child;
                break;
            }
        }
    }

    Node node{
        .name = std::string(name),
        .parent = parent,
        .bp = bp,
        .kind = kind
    };

    const NodeId id = nodeSystem_.registry.create(std::move(node));

    if (previous != InvalidNodeId) {
        const RoleId role = nodeSystem_.context.role(nodeSystem_.blueprints.require(bp).name);
        const ContextScopeId scope = nodeSystem_.context.createScope(parent);

        if (!nodeSystem_.context.lookup(scope, role).has_value())
            nodeSystem_.context.set(scope, role, previous);
    }

    return id;
}

NodeId NodeFactory::folder(NodeId parent, std::string_view name) {
    return createNode(parent, name, InvalidBlueprintId, NodeKind::Folder);
}

void* NodeFactory::resolve(NodeId id, BlueprintId api) const {
    const auto& object = nodeSystem_.registry.require(id).object;
    return nodeSystem_.blueprints.cast(object.bp, api, object.ptr);
}

NodeId NodeFactory::component(NodeId parent, BlueprintId api, std::string_view instance, const void* desc) {
    nodeSystem_.blueprints.require(api);

    const BlueprintId impl = nodeSystem_.blueprints.resolveImplementation(api);

    if (const NodeId existing = nodeSystem_.registry.find(instance, parent, impl); existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);

        if (node.kind == NodeKind::Component && node.bp == impl)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", instance, parent);
    }

    const NodeId id = createNode(parent, instance, impl, NodeKind::Component);
    const auto blueprint = nodeSystem_.blueprints.require(impl);

    try {
        if (!blueprint.meta.create)
            throw Exception("NodeFactory", "Blueprint '{}' has no create callback", blueprint.name);

        void* object = blueprint.meta.create(nodeSystem_.build(id), desc);
        nodeSystem_.registry.require(id).object = {object, impl, false};

        if (!object)
            throw Exception("NodeFactory", "Blueprint '{}' returned null", blueprint.name);

    } catch (...) {
        auto release = [&](auto&& self, NodeId current) -> void {
            const auto children = nodeSystem_.registry.children(current);
            const std::vector<NodeId> copy(children.begin(), children.end());
            for (auto child : copy) self(self, child);
            const auto object = nodeSystem_.registry.require(current).object;
            nodeSystem_.registry.require(current).object = {};
            if (object.ptr) nodeSystem_.blueprints.require(object.bp).meta.destroy(object.ptr);
            nodeSystem_.registry.destroy(current);
        };
        release(release, id);

        throw;
    }

    return id;
}

NodeId NodeFactory::component(NodeId parent, std::string_view blueprint, std::string_view instance, const void* desc) {
    const BlueprintId id = nodeSystem_.blueprints.resolve(blueprint);
    if (id == InvalidBlueprintId)
        throw Exception("NodeFactory", "Unknown blueprint '{}'", blueprint);
    return component(parent, id, instance, desc);
}

NodeId NodeFactory::slot(NodeId parent, std::string_view api, std::string_view instance) {
    const BlueprintId id = nodeSystem_.blueprints.resolve(api);
    if (id == InvalidBlueprintId)
        throw Exception("NodeFactory", "Unknown API '{}'", api);
    return slot(parent, id, instance);
}

NodeId NodeFactory::slot(NodeId parent, BlueprintId api, std::string_view instance) {
    nodeSystem_.blueprints.require(api);

    if (const NodeId existing = nodeSystem_.registry.find(instance, parent, api); existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);
        if (node.kind == NodeKind::Slot && node.bp == api)
            return existing;
    }

    const NodeId id = createNode(parent, instance, api, NodeKind::Slot);

    const RoleId role = nodeSystem_.context.role(nodeSystem_.blueprints.require(api).name);
    const ContextScopeId scope = nodeSystem_.context.createScope(parent);

    if (!nodeSystem_.context.lookup(scope, role).has_value())
        nodeSystem_.context.set(scope, role, id);

    return id;
}

void NodeFactory::choice(NodeId id, BlueprintId impl) {
    const BlueprintId api = nodeSystem_.registry.require(id).bp;
    const BlueprintId current = nodeSystem_.registry.require(id).object.bp;

    if (nodeSystem_.registry.require(id).kind != NodeKind::Slot)
        throw Exception("NodeFactory", "Node #{} is not a slot", id);

    if (!nodeSystem_.blueprints.isA(impl, api))
        throw Exception("NodeFactory", "'{}' does not implement '{}'",
            nodeSystem_.blueprints.require(impl).name,
            nodeSystem_.blueprints.require(api).name);

    if (current == impl)
        return;

    const auto create = nodeSystem_.blueprints.require(impl).meta.create;
    if (!create)
        throw Exception("NodeFactory", "Blueprint '{}' is not constructible", nodeSystem_.blueprints.require(impl).name);

    nodeSystem_.ops.clearContents(id);
    try {
        void* object = create(nodeSystem_.build(id), nullptr);
        if (!object)
            throw Exception("NodeFactory", "Failed to create '{}'", nodeSystem_.blueprints.require(impl).name);

        nodeSystem_.registry.require(id).object = {object, impl, false};
    } catch (...) {
        nodeSystem_.ops.clearContents(id);
        throw;
    }
}

NodeId NodeFactory::binding(NodeId parent, std::string_view name) {
    if (const NodeId existing = nodeSystem_.registry.find(name, parent); existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);

        if (node.kind == NodeKind::Binding)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", name, parent);
    }

    return createNode(parent, name, InvalidBlueprintId, NodeKind::Binding);
}

NodeId NodeFactory::mount(NodeId parent, BlueprintId blueprint, std::string_view instance) {
    nodeSystem_.blueprints.require(blueprint);

    if (const NodeId existing = nodeSystem_.registry.find(instance, parent, blueprint); existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);

        if (node.kind == NodeKind::Mount && node.bp == blueprint)
            return existing;

        throw Exception("NodeFactory", "Node '{}' already exists under parent #{}", instance, parent);
    }

    return createNode(parent, instance, blueprint, NodeKind::Mount);
}

}
