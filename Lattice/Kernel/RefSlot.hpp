#pragma once

#include <string_view>
#include <cstddef>
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

}

using Lattice::Ref;
using Lattice::Slot;
using Lattice::Mount;