#pragma once

#include <string_view>
#include <cstddef>
#include <vector>
#include <Lattice/Kernel/TypeName.hpp>

namespace Lattice {

class Node;

template<typename T>
struct Slot {
    Node* node = nullptr;

    Slot() = default;
    Slot(Node& node) : node(&node) {}

    T* get() const;
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    void use(std::string_view implName);

    template<typename Impl>
    void use() {
        use(typeKey<Impl>());
    }

    bool exists() const;

    explicit operator bool() const {
        return exists();
    }

    bool operator==(std::nullptr_t) const {
        return !exists();
    }

    bool operator!=(std::nullptr_t) const {
        return exists();
    }
};

template<typename T>
struct Ref {
    T* ptr = nullptr;

    Ref() = default;
    Ref(T* ptr) : ptr(ptr) {}

    T* operator->() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }
    T& get() const noexcept { return *ptr; }
    T* getPtr() const noexcept { return ptr; }
    bool exists() const noexcept { return ptr != nullptr; }

    explicit operator bool() const noexcept { return ptr != nullptr; }

    bool operator==(std::nullptr_t) const noexcept { return ptr == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr != nullptr; }
};

template<typename T>
struct Mount final : public Ref<T> {
    Node* node = nullptr;

    Mount() = default;

    Mount(Node& node, T* ptr)
        : Ref<T>(ptr), node(&node) {}

    Node& branch() {
        return *node;
    }

    const Node& branch() const {
        return *node;
    }

    bool exists() const noexcept {
        return this->ptr != nullptr && node != nullptr;
    }

    explicit operator bool() const noexcept {
        return exists();
    }
};

template<typename T>
struct Children {
    std::vector<T*> items;

    Children() = default;
    explicit Children(std::vector<T*> items) : items(&items) {}

    std::size_t size() const noexcept {
        return items->size();
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    bool exists() const noexcept {
        return items != nullptr;
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

/// ссылка на список всех детей типа
/// (автоматически обновляется при изменении ветки)
using Lattice::Children;