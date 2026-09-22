#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <Lattice/Kernel/TypeName.hpp>
#include "Lattice/Kernel/Ids.hpp"

namespace Lattice {

template<typename T>
struct Slot {
    NodeId id = InvalidNodeId;
    T* ptr = nullptr;

    Slot() = default;
    Slot(NodeId node) : id(node) {}
    Slot(NodeId node, T* object) : id(node), ptr(object) {}

    T* get() const noexcept { return ptr; }
    T* operator->() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }

    NodeId getId() const noexcept { return id; }

    void choice(BlueprintId impl);

    template<typename Impl>
    void choice();

    bool exists() const noexcept { return ptr != nullptr; }
    explicit operator bool() const noexcept { return ptr != nullptr; }
};

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

/// ссылка на внешний примонтированный компонент
using Lattice::Mount;

/// Список непосредственных детей типа; обновляется присваиванием в configure.
using Lattice::Children;
