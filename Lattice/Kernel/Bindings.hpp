#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Kernel/Value.hpp>


namespace Lattice {

struct Binding {
    ObjectId id;
    void* object = nullptr;
    Value (*get)(void*) = nullptr;
    void (*set)(Binding*, const Value&) = nullptr;
    std::function<void(const Value&)> invoke;

    double min = 0;
    double max = 0;
    bool hasRange = false;
};

class Bindings {
    static constexpr std::string_view tag = "Bindings";
    std::unordered_map<ObjectId, Binding> bindings;

public:
    template<typename T>
    void bind(ObjectId id, T* ptr, double min = 0, double max = 0, bool hasRange = false) {
        Binding binding;
        binding.id = id;
        binding.object = ptr;
        binding.min = min;
        binding.max = max;
        binding.hasRange = hasRange;

        binding.get = [](void* object) -> Value {
            return Value{*static_cast<T*>(object)};
        };

        binding.set = [](Binding* binding, const Value& value) {
            *static_cast<T*>(binding->object) = value.get<T>();
        };

        bindings[id] = std::move(binding);
    }

    template<typename T, typename F>
    void bind(ObjectId id, T* ptr, F&& onChange, double min = 0, double max = 0, bool hasRange = false) {
        Binding binding;
        binding.id = id;
        binding.object = ptr;
        binding.min = min;
        binding.max = max;
        binding.hasRange = hasRange;
        binding.invoke = [callback = std::forward<F>(onChange)](const Value& value) mutable {
            callback(value.get<T>());
        };

        binding.get = [](void* object) -> Value {
            return Value{*static_cast<T*>(object)};
        };

        binding.set = [](Binding* binding, const Value& value) {
            auto* object = static_cast<T*>(binding->object);
            *object = value.get<T>();

            if (binding->invoke)
                binding->invoke(value);
        };

        bindings[id] = std::move(binding);
    }

    void on(ObjectId id, std::function<void()> handler) {
        Binding binding;
        binding.id = id;
        binding.invoke = [handler = std::move(handler)](const Value&) {
            handler();
        };
        bindings[id] = std::move(binding);
    }

    Binding& get(ObjectId id) {
        return bindings[id];
    }

    void unbind(ObjectId id) {
        bindings.erase(id);
    }
};

}