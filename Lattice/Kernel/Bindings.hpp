// #pragma once

// #include <functional>
// #include <string_view>
// #include <unordered_map>

// #include <Lattice/Kernel/Exception.hpp>
// #include <Lattice/Kernel/Objects.hpp>
// #include <Lattice/Kernel/Value.hpp>

// namespace Lattice {

// struct Binding {
//     void* object = nullptr;

//     Value (*get)(void*) = nullptr;
//     void (*set)(Binding*, const Value&) = nullptr;

//     std::function<void(const Value&)> invoke;

//     double min = 0;
//     double max = 0;
//     bool hasRange = false;
// };

// class Bindings {
//     static constexpr std::string_view tag = "Bindings";

//     std::unordered_map<ObjectId, Binding> bindings;

// public:
//     template<typename T>
//     Binding& bind(ObjectId id, T* ptr, double min = 0, double max = 0, bool hasRange = false) {
//         Binding binding;
//         binding.object = ptr;
//         binding.min = min;
//         binding.max = max;
//         binding.hasRange = hasRange;

//         binding.get = [](void* object) -> Value {
//             return Value{*static_cast<T*>(object)};
//         };

//         binding.set = [](Binding* binding, const Value& value) {
//             *static_cast<T*>(binding->object) = value.get<T>();
//         };

//         bindings[id] = std::move(binding);
//         return bindings[id];
//     }

//     template<typename T, typename F>
//     Binding& bind(ObjectId id, T* ptr, F&& onChange, double min = 0, double max = 0, bool hasRange = false) {
//         Binding binding;
//         binding.object = ptr;
//         binding.min = min;
//         binding.max = max;
//         binding.hasRange = hasRange;

//         binding.invoke = [callback = std::forward<F>(onChange)](const Value& value) mutable {
//             callback(value.get<T>());
//         };

//         binding.get = [](void* object) -> Value {
//             return Value{*static_cast<T*>(object)};
//         };

//         binding.set = [](Binding* binding, const Value& value) {
//             auto* object = static_cast<T*>(binding->object);
//             *object = value.get<T>();

//             if (binding->invoke)
//                 binding->invoke(value);
//         };

//         bindings[id] = std::move(binding);
//         return bindings[id];
//     }

//     Binding& on(ObjectId id, std::function<void()> handler) {
//         Binding binding;

//         binding.invoke = [handler = std::move(handler)](const Value&) {
//             handler();
//         };

//         bindings[id] = std::move(binding);
//         return bindings[id];
//     }

//     Binding* get(ObjectId id) {
//         auto it = bindings.find(id);
//         return it == bindings.end() ? nullptr : &it->second;
//     }

//     const Binding* get(ObjectId id) const {
//         auto it = bindings.find(id);
//         return it == bindings.end() ? nullptr : &it->second;
//     }

//     bool has(ObjectId id) const {
//         return bindings.contains(id);
//     }

//     Value getValue(ObjectId id) const {
//         const auto* binding = get(id);

//         if (!binding || !binding->get)
//             throw Exception("Bindings", "binding for object {} is not readable", id);

//         return binding->get(binding->object);
//     }

//     template<typename T>
//     T get(ObjectId id) const {
//         return getValue(id).get<T>();
//     }

//     void set(ObjectId id, const Value& value) {
//         auto* binding = get(id);

//         if (!binding || !binding->set)
//             throw Exception("Bindings", "binding for object {} is not writable", id);

//         binding->set(binding, value);
//     }

//     template<typename T>
//     void set(ObjectId id, T value) {
//         set(id, Value{std::move(value)});
//     }

//     void invoke(ObjectId id) {
//         auto* binding = get(id);

//         if (!binding || !binding->invoke)
//             return;

//         binding->invoke(Value{});
//     }

//     void unbind(ObjectId id) {
//         bindings.erase(id);
//     }

//     void clear() {
//         bindings.clear();
//     }
// };

// }