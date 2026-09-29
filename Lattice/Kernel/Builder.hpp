#pragma once

#include <span>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>

class NodeBuild;

namespace Lattice {

class NodeFactory;
class NodeSystem;

class Builder {
public:
    class Batch {
    public:
        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;
        Batch(Batch&& other) noexcept;
        Batch& operator=(Batch&&) = delete;
        ~Batch();

        NodeId add(
            NodeId parent,
            BlueprintId blueprint,
            std::string_view instance = DefaultInstanceName,
            const void* descriptor = nullptr
        );

        void commit();
        void rollback() noexcept;

        std::span<const NodeId> branches() const noexcept { return branches_; }

    private:
        friend class Builder;

        explicit Batch(Builder& builder) : builder_(&builder) {}

        Builder* builder_ = nullptr;
        std::vector<NodeId> branches_;
        bool finished_ = false;
    };

    explicit Builder(NodeSystem& nodeSystem) : nodeSystem_(nodeSystem) {}

    Batch begin() { return Batch{*this}; }

    NodeId add(
        NodeId parent,
        BlueprintId blueprint,
        std::string_view instance = DefaultInstanceName,
        const void* descriptor = nullptr
    );

    void del(NodeId parent, NodeId child);

private:
    friend class Batch;
    friend class NodeFactory;

    ::NodeBuild node(NodeId id);

    NodeId create(
        NodeId parent,
        BlueprintId blueprint,
        std::string_view instance,
        const void* descriptor
    );

    NodeSystem& nodeSystem_;
};

}
