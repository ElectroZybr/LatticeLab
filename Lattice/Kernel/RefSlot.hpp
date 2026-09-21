#pragma once

#include <string_view>
#include <cstddef>
#include <vector>
#include <utility>
#include <Lattice/Kernel/TypeName.hpp>
#include <Lattice/Kernel/Objects.hpp>

namespace Lattice {

class Node;
class Context;

template<typename T>
struct Slot {
    Context* ctx = nullptr;
    ObjectId id = InvalidObjectId;

    Slot() = default;
    Slot(Node& node);
    Slot(Context& context, ObjectId object) : ctx(&context), id(object) {}

    T* get() const;
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    Node* node() const;
    ObjectId getId() const noexcept { return id; }

    void use(std::string_view implName);

    template<typename Impl>
    void use() {
        use(typeKey<Impl>());
    }

    Slot& focus(std::string_view role = typeKey<T>());
    template<class Role>
    Slot& focus() { return focus(typeKey<Role>()); }

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
    Context* ctx = nullptr;
    ObjectId id = InvalidObjectId;

    Ref() = default;
    Ref(Node& node);
    Ref(Context& context, ObjectId object) : ctx(&context), id(object) {}

    T* getPtr() const;
    T* operator->() const { return getPtr(); }
    T& operator*() const { return *getPtr(); }
    T& get() const { return *getPtr(); }
    Node* node() const;
    ObjectId getId() const noexcept { return id; }

    Ref& focus(std::string_view role = typeKey<T>());
    template<class Role>
    Ref& focus() { return focus(typeKey<Role>()); }

    bool exists() const { return getPtr() != nullptr; }

    explicit operator bool() const { return exists(); }

    bool operator==(std::nullptr_t) const { return !exists(); }
    bool operator!=(std::nullptr_t) const { return exists(); }
};

template<typename T>
struct Mount final : public Ref<T> {
    Mount() = default;
    using Ref<T>::Ref;

    Node& branch();
    const Node& branch() const;
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
