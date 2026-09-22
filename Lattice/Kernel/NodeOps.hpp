#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Ids.hpp>

namespace Lattice {

class NodeRegistry;
class Blueprints;
class NodeConfigureView;
class NodeQuery;
class Context;

class NodeOps {
    NodeRegistry& nodes_;
    Blueprints& blueprints_;
    NodeQuery& query_;
    Context& context_;

public:
    NodeOps(NodeRegistry& nodes, Blueprints& blueprints, NodeQuery& query, Context& context)
        : nodes_(nodes), blueprints_(blueprints), query_(query), context_(context) {}

    void configureBranch(NodeId id);
    void destroyBranch(NodeId id);
    void configure(NodeId id);

    void dumpTree(NodeId id, NodeId highlighted = InvalidNodeId) const;

    NodeId resolvePath(NodeId from, std::string_view path) const;
    std::string stringPath(NodeId id) const;

    NodeId root(NodeId id) const;
    bool isUnder(NodeId id, NodeId ancestor) const;

    std::vector<NodeId> collectTree(NodeId id) const;
};

}