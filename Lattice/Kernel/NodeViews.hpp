#pragma once

#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeHandlers.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>

namespace Lattice {

class NodeBuildView {
    NodeId id_;
    NodeFactory& factory_;
    Blueprints& blueprints_;
    NodeQuery& query_;

public:
    NodeBuildView(NodeId id, NodeFactory& factory, Blueprints& blueprints, NodeQuery& query)
        : id_(id), factory_(factory), blueprints_(blueprints), query_(query) {}

    NodeId id() const noexcept { return id_; }

    NodeId addFolder(std::string_view name) { return factory_.folder(id_, name); }

    template<typename API>
    Slot<API> slot(std::string_view instance = {}) {
        const NodeId id = factory_.slot(id_, blueprints_.id<API>(), instance);
        return Slot<API>{factory_, query_, blueprints_, id};
    }

    template<class T> Ref<T> 
    add(std::string_view instance = DefaultInstanceName) {
        auto api = blueprints_.find(typeKey<T>());
        auto node = factory_.component(id_, api, instance);
        return Ref<T>{static_cast<T*>(factory_.resolve(node, api))};
    }

    template<class API> 
    Children<API> addImpls() {
        std::vector<API*> result;
        auto api = blueprints_.find(typeKey<API>());

        for (auto impl : blueprints_.getImpls(api)) {
            const std::string name = blueprints_.require(impl).name;
            auto node = factory_.component(id_, impl, name);
            result.push_back(static_cast<API*>(factory_.resolve(node, api)));
        }

        return Children<API>{std::move(result)};
    }
};

class NodeConfigureView {
    NodeId id_;
    NodeQuery& query_;
    NodeContext& context_;

public:
    NodeConfigureView(NodeId id, NodeQuery& query, NodeContext& context)
        : id_(id), query_(query), context_(context) {}

    NodeId id() const noexcept { return id_; }

    template<typename T>
    Focus<T> focus(std::string_view role = typeKey<T>()) {
        return Focus<T>{context_, query_, context_.nearestScope(id_), context_.role(role)};
    }

    NodeId findId(std::string_view api, std::string_view instance = DefaultInstanceName) const;
    NodeId requireId(std::string_view api, std::string_view instance = DefaultInstanceName) const;

    template<class T> Ref<T> 
    find(std::string_view instance = DefaultInstanceName) const {
        auto found = findId(typeKey<T>(), instance);
        return Ref<T>{found == InvalidNodeId ? nullptr : resolve<T>(found)};
    }

    template<class T> Ref<T> 
    require(std::string_view instance = DefaultInstanceName) const {
        return Ref<T>{resolve<T>(requireId(typeKey<T>(), instance))};
    }

    template<class T> T* 
    resolve(NodeId id) const {
        return static_cast<T*>(query_.resolve(id, typeKey<T>()));
    }
};
}
