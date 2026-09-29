#include <algorithm>
#include <exception>
#include <ranges>
#include <Lattice/Kernel/NodeOps.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/TreeFormatter.hpp>

namespace Lattice {

NodeId NodeOps::resolvePath(NodeId from, std::string_view path) const {
    NodeId current = from;

    size_t begin = 0;

    while (begin < path.size()) {
        size_t end = path.find('/', begin);
        if (end == std::string_view::npos)
            end = path.size();

        const std::string_view name = path.substr(begin, end - begin);

        if (!name.empty()) {
            current = nodeSystem_.registry.find(name, current);

            if (current == InvalidNodeId)
                return InvalidNodeId;
        }

        begin = end + 1;
    }

    return current;
}

NodeId NodeOps::root(NodeId id) const {
    nodeSystem_.registry.require(id);

    while (nodeSystem_.registry.require(id).parent != InvalidNodeId)
        id = nodeSystem_.registry.require(id).parent;

    return id;
}

bool NodeOps::isUnder(NodeId id, NodeId ancestor) const {
    while (id != InvalidNodeId) {
        if (id == ancestor)
            return true;

        id = nodeSystem_.registry.require(id).parent;
    }

    return false;
}

std::vector<NodeId> NodeOps::collectTree(NodeId id) const {
    std::vector<NodeId> result;

    auto collect = [&](auto&& self, NodeId current) -> void {
        for (NodeId child : nodeSystem_.registry.children(current)) {
            result.push_back(child);
            self(self, child);
        }
    };

    collect(collect, id);
    return result;
}

std::vector<NodeId> NodeOps::collectRemovalClosure(NodeId id) const {
    nodeSystem_.registry.require(id);

    std::vector<NodeId> result;
    std::vector<bool> included(nodeSystem_.registry.size(), false);

    auto addBranch = [&](auto&& self, NodeId root) -> void {
        const auto* node = nodeSystem_.registry.get(root);
        if (!node || included[root])
            return;

        included[root] = true;
        result.push_back(root);

        for (NodeId child : nodeSystem_.registry.children(root))
            self(self, child);

        if (node->kind == NodeKind::Mount && node->relation != InvalidNodeId)
            self(self, node->relation);
    };

    addBranch(addBranch, id);

    for (size_t index = 0; index < result.size(); ++index) {
        const auto* node = nodeSystem_.registry.get(result[index]);
        if (node && node->kind == NodeKind::SharedMount &&
            node->relation != InvalidNodeId) {
            const auto references = nodeSystem_.registry.references(node->relation);
            if (std::ranges::all_of(references, [&included](NodeId reference) {
                    return reference < included.size() && included[reference];
                }))
                addBranch(addBranch, node->relation);
        }

        for (NodeId dependent : nodeSystem_.dependencies.dependents(result[index]))
            addBranch(addBranch, dependent);
    }

    return result;
}

void NodeOps::retireBranch(NodeId id) {
    if (!nodeSystem_.registry.get(id))
        return;

    std::vector<NodeId> closure = collectRemovalClosure(id);
    std::vector<NodeId> newlyRetiring;

    for (NodeId nodeId : closure) {
        const auto* node = nodeSystem_.registry.get(nodeId);
        if (!node || node->parent == InvalidNodeId)
            continue;

        if (std::ranges::find(closure, node->parent) == closure.end())
            invalidate(node->parent);
    }

    for (NodeId nodeId : closure) {
        auto* node = nodeSystem_.registry.get(nodeId);
        if (!node || node->state == NodeState::Retiring)
            continue;

        node->state = NodeState::Retiring;
        nodeSystem_.context.removeTarget(nodeId);
        nodeSystem_.exports.remove(nodeId);
        newlyRetiring.push_back(nodeId);
    }

    size_t destination = retiring_.size();
    for (size_t index = 0; index < retiring_.size();) {
        const bool overlaps = std::ranges::any_of(retiring_[index], [&closure](NodeId queued) {
            return std::ranges::find(closure, queued) != closure.end();
        });

        if (!overlaps) {
            ++index;
            continue;
        }

        if (destination == retiring_.size()) {
            destination = index;
            ++index;
            continue;
        }

        auto& target = retiring_[destination];
        for (NodeId queued : retiring_[index])
            if (std::ranges::find(target, queued) == target.end())
                target.push_back(queued);
        retiring_.erase(retiring_.begin() + static_cast<std::ptrdiff_t>(index));
    }

    if (destination == retiring_.size()) {
        retiring_.push_back(std::move(closure));
    } else {
        auto& target = retiring_[destination];
        for (NodeId nodeId : closure)
            if (std::ranges::find(target, nodeId) == target.end())
                target.push_back(nodeId);
    }

    std::exception_ptr failure;
    for (NodeId nodeId : newlyRetiring | std::views::reverse) {
        const auto* node = nodeSystem_.registry.get(nodeId);
        if (!node || !node->object.ptr)
            continue;

        const auto* ops = nodeSystem_.blueprints.require(node->object.bp).meta.ops;
        if (!ops || !ops->retire)
            continue;

        try {
            ops->retire(node->object.ptr);
        } catch (...) {
            if (!failure)
                failure = std::current_exception();
        }
    }

    if (failure)
        std::rethrow_exception(failure);
}

size_t NodeOps::collectRetired() {
    size_t collected = 0;

    for (size_t index = 0; index < retiring_.size();) {
        const auto ready = std::ranges::all_of(retiring_[index], [this](NodeId nodeId) {
            const auto* node = nodeSystem_.registry.get(nodeId);
            if (!node || !node->object.ptr)
                return true;

            const auto* ops = nodeSystem_.blueprints.require(node->object.bp).meta.ops;
            return !ops || !ops->readyToDestroy ||
                   ops->readyToDestroy(node->object.ptr);
        });

        if (!ready) {
            ++index;
            continue;
        }

        auto nodes = std::move(retiring_[index]);
        retiring_.erase(retiring_.begin() + static_cast<std::ptrdiff_t>(index));
        ++collected;

        for (NodeId nodeId : nodes | std::views::reverse) {
            if (!nodeSystem_.registry.get(nodeId))
                continue;
            destroyBranch(nodeId);
        }
    }

    return collected;
}

void NodeOps::invalidate(NodeId id) {
    if (id == InvalidNodeId)
        return;

    const auto* node = nodeSystem_.registry.get(id);
    if (!node || node->state == NodeState::Retiring)
        return;

    if (std::ranges::find(invalidated_, id) == invalidated_.end())
        invalidated_.push_back(id);
}

void NodeOps::maintain() {
    collectRetired();

    std::vector<NodeId> pending;
    pending.swap(invalidated_);

    for (size_t index = 0; index < pending.size(); ++index) {
        const NodeId id = pending[index];
        auto* node = nodeSystem_.registry.get(id);
        if (!node || node->state == NodeState::Retiring || !node->object.ptr)
            continue;

        nodeSystem_.dependencies.clearDependencies(id);
        node->object.configured = false;

        try {
            configure(id);
        } catch (...) {
            for (; index < pending.size(); ++index)
                invalidate(pending[index]);
            throw;
        }
    }
}

void NodeOps::configure(NodeId id) {
    auto& node = nodeSystem_.registry.require(id);

    if (node.state == NodeState::Retiring || !node.object.ptr || node.kind == NodeKind::Mount ||
        node.kind == NodeKind::SharedMount || node.object.configured)
        return;

    const auto& blueprint = nodeSystem_.blueprints.require(node.object.bp);

    if (blueprint.meta.configure)
        blueprint.meta.configure(node.object.ptr, NodeConfigure{id, nodeSystem_});

    nodeSystem_.registry.require(id).object.configured = true;
}

void NodeOps::configureBranch(NodeId id) {
    const auto* node = nodeSystem_.registry.get(id);
    if (!node || node->state == NodeState::Retiring)
        return;

    if (node->kind == NodeKind::Mount || node->kind == NodeKind::SharedMount) {
        if (node->relation != InvalidNodeId && nodeSystem_.registry.get(node->relation))
            configureBranch(node->relation);
        return;
    }

    configure(id);

    const auto span = nodeSystem_.registry.children(id);
    const std::vector<NodeId> children(span.begin(), span.end());
    for (NodeId child : children) configureBranch(child);
}

void NodeOps::destroyBranch(NodeId id) {
    auto* node = nodeSystem_.registry.get(id);
    if (!node)
        return;

    if (node->kind == NodeKind::Mount || node->kind == NodeKind::SharedMount) {
        const NodeKind kind = node->kind;
        const NodeId target = node->relation;

        clearContents(id);
        nodeSystem_.context.removeTarget(id);
        nodeSystem_.exports.remove(id);
        if (const ContextScopeId scope = nodeSystem_.context.findScope(id); scope != InvalidContextScopeId)
            nodeSystem_.context.destroyScope(scope);
        nodeSystem_.dependencies.removeNode(id);
        std::erase(invalidated_, id);
        nodeSystem_.registry.destroy(id);

        if (!nodeSystem_.registry.get(target))
            return;

        if (kind == NodeKind::Mount) {
            destroyBranch(target);
            return;
        }

        if (nodeSystem_.registry.references(target).empty())
            destroyBranch(target);
        return;
    }

    // The physical owner wins over both local and shared logical lifetimes.
    // Detach all incoming references before destroying the physical object.
    const auto span = nodeSystem_.registry.references(id);
    const std::vector<NodeId> references(span.begin(), span.end());

    for (NodeId reference : references) {
        nodeSystem_.registry.unlink(reference);
        destroyBranch(reference);
    }

    clearContents(id);

    nodeSystem_.context.removeTarget(id);
    nodeSystem_.exports.remove(id);

    if (const ContextScopeId scope = nodeSystem_.context.findScope(id); scope != InvalidContextScopeId)
        nodeSystem_.context.destroyScope(scope);

    nodeSystem_.dependencies.removeNode(id);
    std::erase(invalidated_, id);
    nodeSystem_.registry.destroy(id);
}

void NodeOps::clearContents(NodeId id) {
    const auto children = std::vector<NodeId>(nodeSystem_.registry.children(id).begin(), nodeSystem_.registry.children(id).end());

    for (NodeId child : children)
        if (nodeSystem_.registry.get(child))
            destroyBranch(child);

    auto& node = nodeSystem_.registry.require(id);

    if (node.object.ptr && node.kind != NodeKind::Mount && node.kind != NodeKind::SharedMount) {
        const auto& bp = nodeSystem_.blueprints.require(node.object.bp);

        if (bp.meta.ops && bp.meta.ops->destroy)
            bp.meta.ops->destroy(node.object.ptr);

        node.object = {};
    }
}

std::string NodeOps::stringPath(NodeId id) const {
    std::vector<NodeId> path;

    for (NodeId current = id; current != InvalidNodeId; current = nodeSystem_.registry.require(current).parent)
        path.push_back(current);

    std::ranges::reverse(path);

    std::string result;

    for (NodeId current : path) {
        const auto& node = nodeSystem_.registry.require(current);

        if (!result.empty())
            result += '/';

        if (node.bp == InvalidBlueprintId) {
            result += node.name;
            continue;
        }

        const auto name = nodeSystem_.blueprints.require(node.bp).shortName();

        if (node.name.empty())
            result += name;
        else
            result += std::format("{}:{}", name, node.name);
    }

    return result;
}

void NodeOps::dumpTree(NodeId id, NodeId highlighted) const {
    const auto& root = nodeSystem_.registry.require(id);
    Lattice::TreeFormatter tree(root.name.empty() ? "Root" : root.name);

    auto append = [&](auto&& self, NodeId current, size_t depth) -> void {
        for (NodeId childId : nodeSystem_.registry.children(current)) {
            const auto& child = nodeSystem_.registry.require(childId);
            std::string line;

            switch (child.kind) {
                case NodeKind::Folder:
                    line = std::format("{} <a>F</>", child.name);
                    break;

                case NodeKind::Component: {
                    const auto type = nodeSystem_.blueprints.require(child.bp).shortName();
                    line = child.name.empty()
                        ? std::format("{} <ok>C</>", type)
                        : std::format("{}<mut2>::{}</> <ok>C</>", type, child.name);
                    break;
                }

                case NodeKind::Slot: {
                    const auto type = nodeSystem_.blueprints.require(child.bp).shortName();
                    line = child.name.empty()
                        ? std::string(type)
                        : std::format("{}<mut2>::{}</>", type, child.name);

                    line += child.object.ptr
                        ? std::format("<mut2>::</><h>{}</> <h>S</>", nodeSystem_.blueprints.require(child.object.bp).shortName())
                        : "<mut2>::</><h>empty</> <h>S</>";
                    break;
                }

                case NodeKind::Binding:
                    line = std::format("{} <wrn>λ</>", child.name);
                    break;

                case NodeKind::Mount:
                    line = child.name.empty()
                        ? std::format("<a>[&{}]</> <h>&</>", nodeSystem_.blueprints.require(child.bp).shortName())
                        : std::format("<a>[&{}]</> <h>&</>", child.name);
                    break;

                case NodeKind::SharedMount:
                    line = child.name.empty()
                        ? std::format("<a>[&&{}]</> <h>&&</>", nodeSystem_.blueprints.require(child.bp).shortName())
                        : std::format("<a>[&&{}]</> <h>&&</>", child.name);
                    break;
            }

            if (childId == highlighted)
                line = std::format("<err>{} 🡸</>", line);

            line += std::format(" <mut2>#{}</>", childId);

            tree.node(line, depth);
            self(self, childId, depth + 1);
        }
    };

    append(append, id, 0);
    Logger::message(tree.format());
    Logger::blank();
}

void NodeOps::dumpContext() const {
    Lattice::TreeFormatter tree("Context");

    const auto label = [this](NodeId id) -> std::string {
        if (id == InvalidNodeId) return "Empty";
        return nodeSystem_.registry.get(id) ? stringPath(id) : std::format("missing #{}", id);
    };

    tree.node("<h>Scopes</>", 0);

    for (ContextScopeId id = 0; id < nodeSystem_.context.scopeCount(); ++id) {
        const auto* scope = nodeSystem_.context.scope(id);
        if (!scope) continue;

        std::string state;
        if (id == nodeSystem_.context.root()) state += " <a>[root]</>";
        if (nodeSystem_.context.isActive(id)) state += " <a>[active]</>";

        const auto type = scope->type == InvalidBlueprintId ? std::string_view{"untyped"} : nodeSystem_.blueprints.require(scope->type).name;

        tree.node(std::format("{}{} <mut2>#{} [{}]</>", label(scope->owner), state, id, type), 1);

        for (const auto& entry : scope->roles)
            tree.node(std::format("{} ➜ <mut2>{}</>", nodeSystem_.context.roleName(entry.role), label(entry.target)), 2);
    }

    tree.node("<h>Resolved roles</>", 0);

    for (RoleId role = 0; role < nodeSystem_.context.roleCount(); ++role) {
        if (!nodeSystem_.context.hasRole(role)) continue;
        tree.node(std::format("{} ➜ <mut2>{}</>", nodeSystem_.context.roleName(role), label(nodeSystem_.context.resolve(role))), 1);
    }

    Logger::message(tree.format());
    Logger::blank();
}

}
