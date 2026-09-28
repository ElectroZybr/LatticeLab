#include <Lattice/Kernel/NodeQuery.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeContext.hpp>

namespace Lattice {

bool NodeQuery::provides(NodeId id, BlueprintId api) const {
    const auto& node = registry_.require(id);

    if ((node.kind == NodeKind::Mount || node.kind == NodeKind::SharedMount) &&
        node.relation != InvalidNodeId)
        return provides(node.relation, api);

    return blueprints_.isA(node.bp, api) ||
           blueprints_.isA(node.object.bp, api);
}

NodeId NodeQuery::find(NodeId from, BlueprintId api, std::string_view instance) const {
    blueprints_.require(api);
    registry_.require(from);

    for (NodeId current = from; current != InvalidNodeId; current = registry_.require(current).parent) {
        NodeId result = InvalidNodeId;
        for (NodeId child : registry_.children(current)) {
            const auto& node = registry_.require(child);

            if (node.name != instance)
                continue;

            if (provides(child, api)) {
                if (result != InvalidNodeId)
                    throw Exception("NodeQuery", "Ambiguous '{}' with instance '{}' under #{}",
                                    blueprints_.require(api).name, instance, current);
                result = child;
            }
        }
        if (result != InvalidNodeId) return result;
    }

    return InvalidNodeId;
}

NodeId NodeQuery::require(NodeId from, BlueprintId api, std::string_view instance) const {
    const NodeId id = find(from, api, instance);

    if (id == InvalidNodeId)
        throw Exception("NodeQuery", "Object '{}' with instance '{}' not found",
                        blueprints_.require(api).name, instance);

    if (!resolve(id, api))
        throw Exception("NodeQuery", "Slot '{}' with instance '{}' is empty",
                        blueprints_.require(api).name, instance);

    return id;
}

NodeId NodeQuery::find(NodeId from, std::string_view api, std::string_view instance) const {
    const BlueprintId id = blueprints_.resolve(api);
    return id == InvalidBlueprintId ? InvalidNodeId : find(from, id, instance);
}

NodeId NodeQuery::require(NodeId from, std::string_view api, std::string_view instance) const {
    const BlueprintId id = blueprints_.resolve(api);

    if (id == InvalidBlueprintId)
        throw Exception("NodeQuery", "Unknown blueprint '{}'", api);

    return require(from, id, instance);
}

std::vector<NodeId> NodeQuery::collect(NodeId from, BlueprintId api) const {
    registry_.require(from);
    blueprints_.require(api);

    std::vector<NodeId> result;

    auto walk = [&](auto&& self, NodeId current) -> void {
        if (provides(current, api))
            result.push_back(current);

        for (NodeId child : registry_.children(current))
            self(self, child);
    };

    walk(walk, from);

    return result;
}

std::vector<NodeId> NodeQuery::collect(NodeId from, std::string_view api) const {
    const BlueprintId id = blueprints_.resolve(api);

    if (id == Blueprints::InvalidId)
        return {};

    return collect(from, id);
}

void* NodeQuery::resolve(NodeId id, BlueprintId api) const {
    const auto& node = registry_.require(id);

    if (node.kind == NodeKind::Mount || node.kind == NodeKind::SharedMount) {
        if (node.relation == InvalidNodeId)
            return nullptr;
        return resolve(node.relation, api);
    }

    if (!node.object.ptr)
        return nullptr;

    return blueprints_.cast(node.object.bp, api, node.object.ptr);
}

void* NodeQuery::resolve(NodeId id, std::string_view api) const {
    const BlueprintId bp = blueprints_.resolve(api);

    if (bp == Blueprints::InvalidId)
        return nullptr;

    return resolve(id, bp);
}

NodeId NodeQuery::shared(NodeId from, BlueprintId api) const {
    const RoleId role = context_.findRole(blueprints_.require(api).name);

    if (role == InvalidRoleId)
        throw Exception("NodeQuery", "Shared '{}' not found", blueprints_.require(api).name);

    const auto scope = context_.nearestScope(from);
    const auto result = context_.resolveInfo(scope, role);

    if (result.state == ContextResolutionState::Missing)
        throw Exception("NodeQuery", "Shared '{}' not found", blueprints_.require(api).name);

    if (result.state == ContextResolutionState::Ambiguous)
        throw Exception("NodeQuery", "Shared '{}' is ambiguous", blueprints_.require(api).name);

    return result.target;
}

}

/*=== NodeConfigure ===*/
Lattice::NodeId NodeConfigure::findId(std::string_view api, std::string_view instance) const {
    return nodeSystem_.query.find(id_, api, instance);
}

Lattice::NodeId NodeConfigure::requireId(std::string_view api, std::string_view instance) const {
    return nodeSystem_.query.require(id_, api, instance);
}
