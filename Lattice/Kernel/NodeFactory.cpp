#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include "Lattice/Kernel/NodeContext.hpp"
#include "Lattice/Kernel/Node.hpp"
#include "Lattice/Kernel/NodeOps.hpp"

namespace Lattice {

NodeId NodeFactory::createNode(NodeId parent, std::string_view name, BlueprintId bp, NodeKind kind) {
    NodeId previous = InvalidNodeId;

    if (parent != InvalidNodeId && bp != InvalidBlueprintId) {
        for (NodeId child : nodes_.children(parent)) {
            if (nodes_.require(child).bp == bp) {
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

    const NodeId id = nodes_.create(std::move(node));

    if (previous != InvalidNodeId) {
        const RoleId role = context_.role(blueprints_.require(bp).name);
        const ContextScopeId scope = context_.createScope(parent);

        if (!context_.lookup(scope, role).has_value())
            context_.set(scope, role, previous);
    }

    return id;
}

NodeId NodeFactory::folder(NodeId parent, std::string_view name) {
    return createNode(parent, name, InvalidBlueprintId, NodeKind::Folder);
}

void* NodeFactory::resolve(NodeId id, BlueprintId api) const {
    const auto& object = nodes_.require(id).object;
    return blueprints_.cast(object.bp, api, object.ptr);
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
    const auto blueprint = blueprints_.require(impl);

    try {
        if (!blueprint.meta.create)
            throw Exception("NodeFactory", "Blueprint '{}' has no create callback", blueprint.name);

        void* object = blueprint.meta.create(NodeBuildView{id, *this, blueprints_, query_}, desc);
        nodes_.require(id).object = {object, impl, false};

        if (!object)
            throw Exception("NodeFactory", "Blueprint '{}' returned null", blueprint.name);

        // if (focus_) focus_->created(id);
    } catch (...) {
        auto release = [&](auto&& self, NodeId current) -> void {
            const auto children = nodes_.children(current);
            const std::vector<NodeId> copy(children.begin(), children.end());
            for (auto child : copy) self(self, child);
            const auto object = nodes_.require(current).object;
            // if (focus_) focus_->removing(current);
            nodes_.require(current).object = {};
            if (object.ptr) blueprints_.require(object.bp).meta.destroy(object.ptr);
            nodes_.destroy(current);
        };
        release(release, id);

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

NodeId NodeFactory::slot(NodeId parent, BlueprintId api, std::string_view instance) {
    blueprints_.require(api);

    if (const NodeId existing = nodes_.find(instance, parent); existing != InvalidNodeId) {
        const auto& node = nodes_.require(existing);
        if (node.kind == NodeKind::Slot && node.bp == api)
            return existing;
    }

    const NodeId id = createNode(parent, instance, api, NodeKind::Slot);

    const RoleId role = context_.role(blueprints_.require(api).name);
    const ContextScopeId scope = context_.createScope(parent);

    if (!context_.lookup(scope, role).has_value())
        context_.set(scope, role, id);

    return id;
}

void NodeFactory::choice(NodeId id, BlueprintId impl) {
    auto& node = nodes_.require(id);

    if (node.kind != NodeKind::Slot)
        throw Exception("NodeFactory", "Node #{} is not a slot", id);

    if (!blueprints_.isA(impl, node.bp))
        throw Exception("NodeFactory", "'{}' does not implement '{}'",
            blueprints_.require(impl).name, blueprints_.require(node.bp).name);

    if (node.object.bp == impl)
        return;

    ops_.clearContents(id);

    auto& meta = blueprints_.require(impl).meta;

    if (!meta.create)
        throw Exception("NodeFactory", "Blueprint '{}' is not constructible",
            blueprints_.require(impl).name);

    void* object = meta.create(NodeBuildView{id, *this, blueprints_, query_}, nullptr);

    if (!object)
        throw Exception("NodeFactory", "Failed to create '{}'",
            blueprints_.require(impl).name);

    node.object = {object, impl, false};
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
