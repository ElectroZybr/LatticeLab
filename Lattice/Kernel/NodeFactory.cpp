#include <format>
#include <string>
#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/NodeOps.hpp>
#include "Lattice/Kernel/Blueprints.hpp"
#include "Lattice/Kernel/Consts.hpp"
#include "Lattice/Tools/Logger.hpp"

namespace Lattice {

namespace {
constexpr uint64_t NormalIdentity = std::numeric_limits<uint64_t>::max();
constexpr uint64_t SharedIdentity = NormalIdentity - 1;
}

NodeId NodeFactory::createNode(
    NodeId parent,
    std::string_view name,
    BlueprintId bp,
    NodeKind kind,
    uint64_t discriminator
) {
    NodeId previous = InvalidNodeId;

    if (kind == NodeKind::Component && discriminator == NormalIdentity &&
        parent != InvalidNodeId && bp != InvalidBlueprintId) {
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

    const NodeId id = nodeSystem_.registry.create(std::move(node), discriminator);

    if (previous != InvalidNodeId) {
        const RoleId role = nodeSystem_.context.role(nodeSystem_.blueprints.require(bp).name);
        const ContextScopeId scope = nodeSystem_.context.createScope(parent);

        nodeSystem_.context.addCandidate(scope, role, previous);
        nodeSystem_.context.addCandidate(scope, role, id);
    }

    std::string label;
    
    if (bp != InvalidBlueprintId) 
        label += nodeSystem_.blueprints.require(bp).name;
    if (!name.empty())
        label += std::format(":{}", name);

    Logger::info("NodeFactory", "added node '{}'", label);

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
        nodeSystem_.ops.destroyBranch(id);

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

    nodeSystem_.context.addCandidate(scope, role, id);

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

void NodeFactory::share(NodeId id, BlueprintId api) {
    const auto& node = nodeSystem_.registry.require(id);
    nodeSystem_.blueprints.require(api);

    if (!nodeSystem_.blueprints.isA(node.bp, api))
        throw Exception("NodeFactory", "'{}' does not implement '{}'",
            nodeSystem_.blueprints.require(node.bp).name,
            nodeSystem_.blueprints.require(api).name);

    NodeId root = id;
    while (nodeSystem_.registry.require(root).parent != InvalidNodeId)
        root = nodeSystem_.registry.require(root).parent;

    const ContextScopeId scope = nodeSystem_.context.createScope(root);
    const RoleId role = nodeSystem_.context.role(nodeSystem_.blueprints.require(api).name);

    nodeSystem_.context.addCandidate(scope, role, id);
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

NodeId NodeFactory::resource(
    NodeId target,
    BlueprintId api,
    std::string_view instance,
    const void* desc,
    uint64_t discriminator
) {
    nodeSystem_.registry.require(target);
    nodeSystem_.blueprints.require(api);

    if (const NodeId existing = nodeSystem_.registry.find(instance, target, api, discriminator);
        existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);
        if (node.kind == NodeKind::Component && node.bp == api && node.object.ptr)
            return existing;
        throw Exception("NodeFactory", "Mounted resource '{}' has an invalid node", instance);
    }

    const BlueprintId impl = nodeSystem_.blueprints.resolveImplementation(api);
    const NodeId id = createNode(target, instance, api, NodeKind::Component, discriminator);
    const auto blueprint = nodeSystem_.blueprints.require(impl);

    try {
        if (!blueprint.meta.create)
            throw Exception("NodeFactory", "Blueprint '{}' has no create callback", blueprint.name);

        void* object = blueprint.meta.create(nodeSystem_.build(id), desc);
        nodeSystem_.registry.require(id).object = {object, impl, false};
        if (!object)
            throw Exception("NodeFactory", "Blueprint '{}' returned null", blueprint.name);
    } catch (...) {
        nodeSystem_.ops.destroyBranch(id);
        throw;
    }

    return id;
}

NodeId NodeFactory::reference(
    NodeId caller,
    NodeId target,
    BlueprintId api,
    std::string_view instance,
    NodeKind kind
) {
    nodeSystem_.registry.require(caller);
    const auto& physical = nodeSystem_.registry.require(target);

    if (kind != NodeKind::Mount && kind != NodeKind::SharedMount)
        throw Exception("NodeFactory", "Invalid mount reference kind");

    if (!nodeSystem_.blueprints.isA(physical.object.bp, api))
        throw Exception("NodeFactory", "Mounted object does not implement requested API");

    const uint64_t discriminator = physical.parent;
    if (const NodeId existing = nodeSystem_.registry.find(instance, caller, api, discriminator);
        existing != InvalidNodeId) {
        const auto& node = nodeSystem_.registry.require(existing);
        if (node.kind == kind && node.relation == target)
            return existing;
        throw Exception("NodeFactory", "Node '{}' already exists under caller #{}", instance, caller);
    }

    const NodeId id = createNode(caller, instance, api, kind, discriminator);
    nodeSystem_.registry.link(id, target);
    return id;
}

NodeId NodeFactory::addLocal(
    NodeId caller,
    NodeId target,
    BlueprintId api,
    std::string_view instance,
    const void* desc
) {
    nodeSystem_.registry.require(caller);
    nodeSystem_.registry.require(target);

    if (const NodeId existing = nodeSystem_.registry.find(instance, caller, api, target);
        existing != InvalidNodeId) {
        const auto& referenceNode = nodeSystem_.registry.require(existing);
        if (referenceNode.kind == NodeKind::Mount &&
            nodeSystem_.registry.get(referenceNode.relation) &&
            nodeSystem_.registry.require(referenceNode.relation).parent == target)
            return referenceNode.relation;
        throw Exception("NodeFactory", "Local resource '{}' conflicts under caller #{}", instance, caller);
    }

    const uint64_t discriminator = caller;
    const bool created = nodeSystem_.registry.find(instance, target, api, discriminator) == InvalidNodeId;
    const NodeId physical = resource(target, api, instance, desc, discriminator);

    try {
        reference(caller, physical, api, instance, NodeKind::Mount);
    } catch (...) {
        if (created && nodeSystem_.registry.get(physical))
            nodeSystem_.ops.destroyBranch(physical);
        throw;
    }
    return physical;
}

NodeId NodeFactory::addShare(
    NodeId caller,
    NodeId target,
    BlueprintId api,
    std::string_view instance,
    const void* desc
) {
    nodeSystem_.registry.require(caller);
    nodeSystem_.registry.require(target);

    if (const NodeId existing = nodeSystem_.registry.find(instance, caller, api, target);
        existing != InvalidNodeId) {
        const auto& referenceNode = nodeSystem_.registry.require(existing);
        if (referenceNode.kind == NodeKind::SharedMount &&
            nodeSystem_.registry.get(referenceNode.relation) &&
            nodeSystem_.registry.require(referenceNode.relation).parent == target)
            return referenceNode.relation;
        throw Exception("NodeFactory", "Shared resource '{}' conflicts under caller #{}", instance, caller);
    }

    const bool created = nodeSystem_.registry.find(instance, target, api, SharedIdentity) == InvalidNodeId;
    const NodeId physical = resource(target, api, instance, desc, SharedIdentity);

    try {
        reference(caller, physical, api, instance, NodeKind::SharedMount);
    } catch (...) {
        if (created && nodeSystem_.registry.get(physical))
            nodeSystem_.ops.destroyBranch(physical);
        throw;
    }
    return physical;
}

}
