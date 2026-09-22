#include <Lattice/Kernel/NodeOps.hpp>
#include <Lattice/Kernel/NodeRegistry.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Tools/LogTree.hpp>

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
            current = nodes_.find(name, current);

            if (current == InvalidNodeId)
                return InvalidNodeId;
        }

        begin = end + 1;
    }

    return current;
}

NodeId NodeOps::root(NodeId id) const {
    nodes_.require(id);

    while (nodes_.require(id).parent != InvalidNodeId)
        id = nodes_.require(id).parent;

    return id;
}

bool NodeOps::isUnder(NodeId id, NodeId ancestor) const {
    while (id != InvalidNodeId) {
        if (id == ancestor)
            return true;

        id = nodes_.require(id).parent;
    }

    return false;
}

std::vector<NodeId> NodeOps::collectTree(NodeId id) const {
    std::vector<NodeId> result;

    auto collect = [&](auto&& self, NodeId current) -> void {
        for (NodeId child : nodes_.children(current)) {
            result.push_back(child);
            self(self, child);
        }
    };

    collect(collect, id);
    return result;
}

void NodeOps::configure(NodeId id) {
    auto& node = nodes_.require(id);

    if (!node.object.ptr || node.kind == NodeKind::Mount || node.object.configured)
        return;

    const auto& blueprint = blueprints_.require(node.object.bp);

    if (blueprint.meta.configure)
        blueprint.meta.configure(node.object.ptr, NodeConfigureView{id, query_});

    node.object.configured = true;
}

void NodeOps::configureBranch(NodeId id) {
    configure(id);

    for (NodeId child : nodes_.children(id))
        configureBranch(child);
}

void NodeOps::destroyBranch(NodeId id) {
    nodes_.require(id);

    auto destroyObject = [&](NodeId nodeId) {
        auto& node = nodes_.require(nodeId);

        if (!node.object.ptr || node.kind == NodeKind::Mount)
            return;

        const auto& blueprint = blueprints_.require(node.object.bp);

        if (blueprint.meta.destroy)
            blueprint.meta.destroy(node.object.ptr);

        node.object.ptr = nullptr;
        node.object.bp = InvalidBlueprintId;
        node.object.configured = false;
    };

    auto children = collectTree(id);

    for (auto it = children.rbegin(); it != children.rend(); ++it)
        destroyObject(*it);

    destroyObject(id);

    nodes_.destroy(id);
}

std::string NodeOps::stringPath(NodeId id) const {
    std::vector<NodeId> path;

    for (NodeId current = id; current != InvalidNodeId; current = nodes_.require(current).parent)
        path.push_back(current);

    std::ranges::reverse(path);

    std::string result;

    for (NodeId current : path) {
        const auto& node = nodes_.require(current);

        if (!result.empty())
            result += '/';

        if (node.bp == InvalidBlueprintId) {
            result += node.name;
            continue;
        }

        const auto name = blueprints_.require(node.bp).shortName();

        if (node.name.empty())
            result += name;
        else
            result += std::format("{}:{}", name, node.name);
    }

    return result;
}

void NodeOps::dumpTree(NodeId id, NodeId highlighted) const {
    const auto& root = nodes_.require(id);
    Logger::Tree tree(root.name.empty() ? "Root" : root.name);

    auto append = [&](auto&& self, NodeId current, size_t depth) -> void {
        for (NodeId childId : nodes_.children(current)) {
            const auto& child = nodes_.require(childId);
            std::string line;

            switch (child.kind) {
                case NodeKind::Folder:
                    line = std::format("{} <m>F</>", child.name);
                    break;

                case NodeKind::Component: {
                    const auto type = blueprints_.require(child.bp).shortName();
                    line = child.name.empty()
                        ? std::format("{} <g>C</>", type)
                        : std::format("{}<gr>::{}</> <g>C</>", type, child.name);
                    break;
                }

                case NodeKind::Slot: {
                    const auto type = blueprints_.require(child.bp).shortName();
                    line = child.name.empty()
                        ? std::string(type)
                        : std::format("{}<gr>::{}</>", type, child.name);

                    line += child.object.ptr
                        ? std::format("<gr>::<c>{}<//> <c>S</>", blueprints_.require(child.object.bp).shortName())
                        : "<gr>::<c>empty<//> <c>S</>";
                    break;
                }

                case NodeKind::Binding:
                    line = std::format("{} <y>λ</>", child.name);
                    break;

                case NodeKind::Mount:
                    line = child.name.empty()
                        ? std::format("<m>[&{}]</> <bl>&</>", blueprints_.require(child.bp).shortName())
                        : std::format("<m>[&{}]</> <bl>&</>", child.name);
                    break;
            }

            if (childId == highlighted)
                line = std::format("<b><r>{} 🡸<//>", line);

            line += std::format(" <gr>#{}</>", childId);

            tree.node(line, depth);
            self(self, childId, depth + 1);
        }
    };

    append(append, id, 0);
    tree.print();
}

}