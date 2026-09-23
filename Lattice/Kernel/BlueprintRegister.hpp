#pragma once

#include <string_view>
#include <string>
#include <type_traits>

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
            return std::is_constructible_v<T, NodeBuildView, const typename T::Desc&>;
        return std::is_constructible_v<T, NodeBuildView> || std::is_default_constructible_v<T>;
    }();

    if constexpr (constructible) {
        blueprint.meta.create = [](NodeBuildView node, const void* desc) -> void* {
            if constexpr (requires { typename T::Desc; }) {
                if (desc) return new T(node, *static_cast<const typename T::Desc*>(desc));
                if constexpr (std::is_default_constructible_v<typename T::Desc>)
                    return new T(node, typename T::Desc{});
                throw Exception("BlueprintTypes", "'{}' requires a descriptor", typeKey<T>());
            } else if constexpr (std::is_constructible_v<T, NodeBuildView>)
                return new T(node);
            else
                return new T();
        };

        blueprint.meta.destroy = [](void* object) {
            delete static_cast<T*>(object);
        };

        if constexpr (requires(T& object, NodeConfigureView node) { object.configure(node); })
            blueprint.meta.configure = [](void* object, NodeConfigureView node) {
                static_cast<T*>(object)->configure(node);
            };
    }

    return blueprints.create(std::move(blueprint), std::string(name));
}

}