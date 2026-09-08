#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Kernel/Value.hpp>

namespace Lattice {

class Settings {
    static constexpr std::string_view tag = "Settings";

    struct Property {
        double min = 0;
        double max = 0;
        bool hasRange = false;
        std::function<Value()> get;
        std::function<void(const Value&)> set;
    };

    struct Action {
        std::function<void()> handler;
    };

    std::unordered_map<ObjectId, Property> properties_;
    std::unordered_map<ObjectId, Action>   actions_;

public:
    template<typename T>
    void bind(ObjectId id, T* ptr, double min = 0, double max = 0, bool hasRange = false) {
        Property p;
        p.min = min;
        p.max = max;
        p.hasRange = hasRange;
        p.get = [ptr] { return Value{*ptr}; };
        p.set = [ptr](const Value& v) { *ptr = v.get<T>(); };
        properties_[id] = std::move(p);
    }

    template<typename T, typename F>
    void bind(ObjectId id, T* ptr, F&& onChange,
            double min = 0, double max = 0, bool hasRange = false) {
        Property p;
        p.min = min;
        p.max = max;
        p.hasRange = hasRange;
        p.get = [ptr] {
            if constexpr (std::is_floating_point_v<T>)
                return Value{static_cast<double>(*ptr)};
            else if constexpr (std::is_same_v<T, bool>)
                return Value{*ptr};
            else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
                return Value{static_cast<int64_t>(*ptr)};
            else
                return Value{*ptr};
        };
        p.set = [ptr, onChange = std::forward<F>(onChange)](const Value& v) mutable {
            *ptr = v.get<T>();
            onChange(*ptr);
        };
        properties_[id] = std::move(p);
    }

    void on(ObjectId id, std::function<void()> handler) {
        actions_[id] = Action{std::move(handler)};
    }

    template<typename T>
    T get(ObjectId id) const {
        auto it = properties_.find(id);
        if (it == properties_.end() || !it->second.get)
            throw Exception(tag, "no property {}", id);
        return it->second.get().get<T>();
    }

    template<typename T>
    void set(ObjectId id, T value) {
        auto it = properties_.find(id);
        if (it == properties_.end() || !it->second.set)
            throw Exception(tag, "no property {}", id);
        it->second.set(Value{std::move(value)});
    }

    void fire(ObjectId id) const {
        auto it = actions_.find(id);
        if (it == actions_.end() || !it->second.handler)
            throw Exception(tag, "no action {}", id);
        it->second.handler();
    }

    void unbind(ObjectId id) {
        properties_.erase(id);
        actions_.erase(id);
    }
};

} // namespace Lattice