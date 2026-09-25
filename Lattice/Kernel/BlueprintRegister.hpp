#pragma once

#include <string_view>
#include <string>
#include <type_traits>
#include <utility>

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace Lattice::BlueprintRegister {

template<typename T, typename... Bases>
BlueprintId add(Blueprints& blueprints, std::string_view name = typeKey<T>()) {
    static_assert((std::is_convertible_v<T*, Bases*> && ...),
        "Blueprint bases must be public and unambiguous");

    Blueprint blueprint{
        .name = std::string(name),
        .bases = {blueprints.id<Bases>()...}
    };

    blueprint.upcasts = {
        +[](void* ptr) -> void* {
            return static_cast<Bases*>(static_cast<T*>(ptr));
        }...
    };

    if constexpr (requires { typename T::Desc; })
        blueprint.meta.descriptor = typeKey<typename T::Desc>();

    constexpr bool constructible = [] {
        if constexpr (requires { typename T::Desc; })
            return std::is_constructible_v<T, ::NodeBuild, const typename T::Desc&>;
        return std::is_constructible_v<T, ::NodeBuild> || std::is_default_constructible_v<T>;
    }();

    if constexpr (constructible) {
        blueprint.meta.create = [](::NodeBuild node, const void* desc) -> void* {
            if constexpr (requires { typename T::Desc; }) {
                if (desc)
                    return new T(std::move(node), *static_cast<const typename T::Desc*>(desc));

                if constexpr (std::is_default_constructible_v<typename T::Desc>)
                    return new T(std::move(node), typename T::Desc{});

                throw Exception("BlueprintTypes", "'{}' requires a descriptor", typeKey<T>());
            } else if constexpr (std::is_constructible_v<T, ::NodeBuild>) {
                return new T(std::move(node));
            } else {
                return new T();
            }
        };

        blueprint.meta.destroy = [](void* object) {
            delete static_cast<T*>(object);
        };

        if constexpr (requires(T& object, ::NodeConfigure node) { object.configure(node); })
            blueprint.meta.configure = [](void* object, ::NodeConfigure node) {
                static_cast<T*>(object)->configure(node);
            };
    }

    return blueprints.create(std::move(blueprint), std::string(name));
}

}