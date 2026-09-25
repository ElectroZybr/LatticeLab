#pragma once

#include <cassert>
#include <cstddef>
#include <vector>
#include <utility>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/TypeName.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/NodeContext.hpp>
#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>

namespace Lattice {

template<typename T>
struct Ref {
    T* ptr = nullptr;

    Ref() = default;
    Ref(T* object) : ptr(object) {}

    T* get() const noexcept { return ptr; }
    T* operator->() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }

    bool exists() const noexcept { return ptr != nullptr; }
    explicit operator bool() const noexcept { return ptr != nullptr; }
};


template<typename T>
class Slot {
    NodeFactory* factory_;
    Blueprints* blueprints_;
    NodeQuery* query_;
    NodeId id_ = InvalidNodeId;

public:
    Slot() = default;
    Slot(NodeFactory& factory, NodeQuery& query, Blueprints& blueprints, NodeId id)
        : factory_(&factory), query_(&query), blueprints_(&blueprints), id_(id) {}

    bool valid() const noexcept {
        return id_ != InvalidNodeId;
    }

    T* get() const {
        // debug assert
        assert(id_ != InvalidNodeId && "Slot is not initialized"); 
        return static_cast<T*>(query_->resolve(id_, typeKey<T>()));
    }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    NodeId id() const noexcept { return id_; }

    bool exists() const { return valid() && get() != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

    void choice(BlueprintId impl) {
        assert(id_ != InvalidNodeId && "Slot is not initialized");
        factory_->choice(id_, impl);
    }

    template<typename Impl>
    void choice() {
        assert(id_ != InvalidNodeId && "Slot is not initialized");
        factory_->choice(id_, blueprints_->id<Impl>());
    }
};


template<typename T>
class Focus {
    NodeContext* context_ = nullptr;
    NodeQuery* query_ = nullptr;
    ContextScopeId scope_ = InvalidContextScopeId;
    RoleId role_ = InvalidRoleId;

public:
    Focus() = default;
    Focus(NodeContext& context, NodeQuery& query, ContextScopeId scope, RoleId role)
        : context_(&context), query_(&query), scope_(scope), role_(role) {}

    bool valid() const noexcept {
        return role_ != InvalidRoleId;
    }

    NodeId id() const {
        assert(valid() && "Focus is not initialized");

        return scope_ == InvalidContextScopeId
            ? context_->resolve(role_)
            : context_->resolve(scope_, role_);
    }

    T* get() const {
        assert(valid() && "Focus is not initialized");

        const NodeId target = id();
        return target == InvalidNodeId
            ? nullptr
            : static_cast<T*>(query_->resolve(target, typeKey<T>()));
    }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    explicit operator bool() const {
        return valid() && get() != nullptr;
    }

    void choice(NodeId target) {
        assert(valid() && "Focus is not initialized");

        if (scope_ == InvalidContextScopeId)
            throw Exception("Focus", "Cannot modify global focus without a scope");

        context_->set(scope_, role_, target);
    }

    void reset() {
        assert(valid() && "Focus is not initialized");

        if (scope_ == InvalidContextScopeId)
            throw Exception("Focus", "Cannot reset global focus without a scope");

        context_->reset(scope_, role_);
    }
};


template<typename T>
struct Mount final : public Ref<T> {
    Mount() = default;
    using Ref<T>::Ref;
};


template<typename T>
struct Children {
    std::vector<T*> items;

    Children() = default;
    explicit Children(std::vector<T*> items) : items(std::move(items)) {}

    std::size_t size() const noexcept {
        return items.size();
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    bool exists() const noexcept {
        return !empty();
    }

    T* operator[](std::size_t i) const {
        return items[i];
    }

    auto begin() const {
        return items.begin();
    }

    auto end() const {
        return items.end();
    }

    explicit operator bool() const noexcept {
        return exists();
    }
};

}

/// ссылка на объект дерева
using Lattice::Ref;

/// ссылка на слот апи в котором может находится его реализация
using Lattice::Slot;

/// хранит ссылку на текущий объект, позволяет переключать фокус; обновляется присваиванием в configure
using Lattice::Focus;

/// ссылка на внешний примонтированный компонент
using Lattice::Mount;

/// Список непосредственных детей типа; обновляется присваиванием в configure
using Lattice::Children;
