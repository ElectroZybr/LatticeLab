#pragma once

#include <Lattice/Kernel/Ids.hpp>
#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeHandlers.hpp>
#include "Lattice/Kernel/Blueprints.hpp"
#include "Lattice/Kernel/BlueprintsTypes.hpp"
#include "Lattice/Kernel/NodeQuery.hpp"

namespace Lattice {

class NodeRegistry;
class Context;

class NodeBuildView {
    NodeId id_;
    NodeFactory& factory_;
    Blueprints& blueprints_;

public:
    NodeBuildView(NodeId id, NodeFactory& factory, Blueprints& blueprints)
        : id_(id), factory_(factory), blueprints_(blueprints) {}

    NodeId id() const noexcept { return id_; }

    NodeId addFolder(std::string_view name);

    template<typename T>
    Ref<T> add(std::string_view instance = DefaultInstanceName) {
        return Ref<T>{factory_.component(id_, typeKey<T>(), instance)};
    }

    template<typename API>
    Slot<API> slot(std::string_view instance = DefaultInstanceName) {
        return Slot<API>{factory_.slot(id_, typeKey<API>(), instance)};
    }

    template<typename API>
    Children<API> addImpls() {
        std::vector<API*> result;
        const BlueprintId api = BlueprintTypes::id<API>(blueprints_);

        // for (BlueprintId impl : blueprints_.getImpls(api)) {
        //     const NodeId node = factory_.component(id_, impl);

        //     if (auto* ptr = static_cast<API*>(query_.resolve(node, api)))
        //         result.push_back(ptr);
        // }

        return Children<API>{std::move(result)};
    }
};

class NodeConfigureView {
    NodeId id_;
    NodeQuery* query_;

public:
    NodeConfigureView(NodeId id, NodeQuery& query)
        : id_(id), query_(&query) {}

    NodeId id() const noexcept { return id_; }

    NodeId findId(std::string_view api, std::string_view instance = DefaultInstanceName) const;
    NodeId requireId(std::string_view api, std::string_view instance = DefaultInstanceName) const;

    template<typename T>
    Slot<T> find(std::string_view instance = DefaultInstanceName) const {
        const NodeId found = findId(typeKey<T>(), instance);

        if (found == InvalidNodeId)
            return {};

        return Slot<T>{found, resolve<T>(found)};
    }

    template<typename T>
    Ref<T> require(std::string_view instance = DefaultInstanceName) const {
        const NodeId found = requireId(typeKey<T>(), instance);
        return Ref<T>{resolve<T>(found)};
    }

    template<typename T>
    T* resolve(NodeId id) const {
        return static_cast<T*>(query_->resolve(id, typeKey<T>()));
    }
};
}