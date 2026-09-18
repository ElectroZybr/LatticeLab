#pragma once

#include <algorithm>
#include <type_traits>
#include <Lattice/Kernel/TypeName.hpp>
#include <Lattice/Tools/LogTree.hpp>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/ObjectRegistry.hpp>

namespace Lattice {

class Node;
using BlueprintId = uint32_t;

struct BlueprintMeta {
    std::string_view descriptor;
    void* (*create)(Node&, const void*) = nullptr;
    void (*destroy)(void*) = nullptr;
    void (*configure)(void*, Node&) = nullptr;
};

struct Blueprint {
    std::string name;
    std::vector<BlueprintId> bases;
    bool exists = true;
    BlueprintMeta meta;
    std::vector<void* (*)(void*)> upcasts;

    std::string_view shortName() const noexcept {
        const auto pos = name.rfind("::");
        return std::string_view(name).substr(pos == std::string::npos ? 0 : pos + 2);
    }

    std::string_view namespaceName() const noexcept {
        const auto pos = name.rfind("::");
        return pos == std::string::npos ? std::string_view{} : std::string_view(name).substr(0, pos);
    }
};

class Blueprints {
    using Registry = ObjectRegistry<Blueprint, BlueprintId, std::string>;
public:
    static constexpr BlueprintId InvalidId = Registry::InvalidId;

    BlueprintId add(std::string_view name, std::initializer_list<BlueprintId> bases = {}) {
        validateName(name);
        std::vector<BlueprintId> uniqueBases;
        for (const auto base : bases) {
            require(base);
            if (std::find(uniqueBases.begin(), uniqueBases.end(), base) == uniqueBases.end())
                uniqueBases.push_back(base);
        }
        return registry_.create({std::string(name), std::move(uniqueBases)});
    }

    template<typename T, typename... Bases>
    BlueprintId add(std::string_view name = typeKey<T>()) {
        static_assert((std::is_convertible_v<T*, Bases*> && ...),
                      "Blueprint bases must be public and unambiguous");
        Blueprint blueprint{std::string(name), {find(typeKey<Bases>())...}};
        validateName(name);
        for (auto base : blueprint.bases)
            require(base);
        blueprint.upcasts = {+[](void* ptr) -> void* {
            return static_cast<Bases*>(static_cast<T*>(ptr));
        }...};
        if constexpr (requires { typename T::Desc; })
            blueprint.meta.descriptor = typeKey<typename T::Desc>();
        constexpr bool constructible = [] {
            if constexpr (requires { typename T::Desc; })
                return std::is_constructible_v<T, Node&, const typename T::Desc&>;
            else
                return std::is_constructible_v<T, Node&> || std::is_default_constructible_v<T>;
        }();
        if constexpr (constructible) {
            blueprint.meta.create = [](Node& node, const void* desc) -> void* {
                if constexpr (requires { typename T::Desc; }) {
                    if (desc)
                        return new T(node, *static_cast<const typename T::Desc*>(desc));
                    if constexpr (std::is_default_constructible_v<typename T::Desc>)
                        return new T(node, typename T::Desc{});
                    else
                        throw Exception("Blueprints", "'{}' requires a descriptor", typeKey<T>());
                } else if constexpr (std::is_constructible_v<T, Node&>) {
                    return new T(node);
                } else {
                    return new T();
                }
            };
            blueprint.meta.destroy = [](void* object) { delete static_cast<T*>(object); };
            if constexpr (requires(T& object, Node& node) { object.configure(node); })
                blueprint.meta.configure = [](void* object, Node& node) { static_cast<T*>(object)->configure(node); };
        }
        return registry_.create(std::move(blueprint));
    }

    BlueprintId resolveImplementation(BlueprintId api, std::string_view nameSpace = {},
                                      std::string_view descriptor = {}) const {
        const auto compatible = [&](BlueprintId id) {
            const auto& type = require(id);
            return type.meta.create && isA(id, api) &&
                (descriptor.empty() || type.meta.descriptor == descriptor);
        };
        if (compatible(api))
            return api;
        const auto select = [&](bool local) {
            BlueprintId result = InvalidId;
            for (BlueprintId id = 0; id < size(); ++id) {
                if (!compatible(id) || (local && require(id).namespaceName() != nameSpace))
                    continue;
                if (result != InvalidId)
                    throw Exception("Blueprints", "Multiple implementations of '{}': '{}' and '{}'",
                                    require(api).name, require(result).name, require(id).name);
                result = id;
            }
            return result;
        };
        if (!nameSpace.empty()) {
            const auto local = select(true);
            if (local != InvalidId)
                return local;
        }
        const auto result = select(false);
        if (result == InvalidId)
            throw Exception("Blueprints", "No constructible implementation of '{}'", require(api).name);
        return result;
    }

    void* cast(BlueprintId from, BlueprintId to, void* object) const {
        if (!object || !get(from) || !get(to))
            return nullptr;
        void* result = nullptr;
        auto walk = [&](auto&& self, BlueprintId id, void* ptr) -> void {
            if (id == to) {
                if (result && result != ptr)
                    throw Exception("Blueprints", "Ambiguous conversion '{}' -> '{}'", require(from).name, require(to).name);
                result = ptr;
                return;
            }
            const auto& type = require(id);
            for (size_t i = 0; i < type.upcasts.size(); ++i)
                if (type.upcasts[i])
                    self(self, type.bases[i], type.upcasts[i](ptr));
        };
        walk(walk, from, object);
        return result;
    }

    void dumpTree() const {
        Logger::Tree tree("Blueprints");
        auto append = [&](auto&& self, BlueprintId id, size_t depth) -> void {
            tree.node(std::format("{} <c>B</> <gr>#{}</>", require(id).name, id), depth);
            for (BlueprintId child = 0; child < size(); ++child) {
                const auto& bases = require(child).bases;
                if (std::find(bases.begin(), bases.end(), id) != bases.end())
                    self(self, child, depth + 1);
            }
        };
        for (BlueprintId id = 0; id < size(); ++id)
            if (require(id).bases.empty())
                append(append, id, 0);
        tree.print();
    }

    BlueprintId find(std::string_view name, std::string_view nameSpace = {}) const {
        if (nameSpace.empty())
            return registry_.find(name);
        return registry_.find(std::string(nameSpace) + "::" + std::string(name));
    }

    // Короткое имя допустимо только при единственном совпадении во всём реестре.
    BlueprintId resolve(std::string_view name) const {
        if (name.find("::") != std::string_view::npos)
            return find(name);

        BlueprintId result = InvalidId;
        for (BlueprintId id = 0; id < size(); ++id) {
            const auto& blueprint = require(id);
            if (blueprint.shortName() != name)
                continue;
            if (result != InvalidId)
                throw Exception("Blueprints", "Ambiguous name '{}': '{}' and '{}'", name, require(result).name, blueprint.name);
            result = id;
        }
        return result;
    }

    const Blueprint* get(BlueprintId id) const { return registry_.get(id); }
    const Blueprint& require(BlueprintId id) const { return registry_.require(id); }
    BlueprintId size() const { return registry_.size(); }

    void addBase(BlueprintId derived, BlueprintId base) {
        require(derived);
        require(base);
        if (isA(base, derived))
            throw Exception("Blueprints", "Inheritance cycle: '{}' -> '{}'", require(derived).name, require(base).name);

        auto& bases = registry_.get(derived)->bases;
        if (std::find(bases.begin(), bases.end(), base) == bases.end())
            bases.push_back(base);
    }

    // Рефлексивное и транзитивное отношение: тип является самим собой.
    // Это проверка графа, не гарантия однозначного C++-приведения указателя.
    bool isA(BlueprintId derived, BlueprintId base) const {
        if (!get(derived) || !get(base))
            return false;

        std::vector<bool> visited(size(), false);
        std::vector<BlueprintId> pending{derived};
        while (!pending.empty()) {
            const auto id = pending.back();
            pending.pop_back();
            if (id == base)
                return true;
            if (visited[id])
                continue;
            visited[id] = true;
            const auto& bases = require(id).bases;
            pending.insert(pending.end(), bases.begin(), bases.end());
        }
        return false;
    }

private:
    static void validateName(std::string_view name) {
        if (name.empty())
            throw Exception("Blueprints", "Blueprint name must not be empty");
        size_t begin = 0;
        while (begin < name.size()) {
            auto end = name.find("::", begin);
            if (end == std::string_view::npos)
                end = name.size();
            const auto part = name.substr(begin, end - begin);
            if (part.empty() || part.find_first_of(": \t\r\n") != std::string_view::npos)
                throw Exception("Blueprints", "Invalid qualified name '{}'", name);
            if (end == name.size())
                return;
            begin = end + 2;
        }
        throw Exception("Blueprints", "Invalid qualified name '{}'", name);
    }

    Registry registry_;
};

} // namespace Lattice
