#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Value.hpp>
#include "Lattice/Tools/LogTree.hpp"

namespace Lattice {

enum class ContextResolutionState : uint8_t;

using ExportId = uint32_t;
inline constexpr ExportId InvalidExportId = std::numeric_limits<ExportId>::max();

enum class ExportKind : uint8_t {
    Param,
    Action
};

struct ExportKey {
    NodeId owner = InvalidNodeId;
    std::string name;

    bool operator==(const ExportKey&) const = default;
};

struct ExportKeyHash {
    size_t operator()(const ExportKey& key) const noexcept {
        return std::hash<NodeId>{}(key.owner) ^ (std::hash<std::string>{}(key.name) << 1);
    }
};

struct Param {
    void* object = nullptr;
    Value (*get)(const void*) = nullptr;
    void (*set)(void*, const Value&) = nullptr;
};

class ActionContext {
public:
    explicit ActionContext(NodeId node = InvalidNodeId) : node_(node) {}

    NodeId node() const noexcept { return node_; }
    void setNode(NodeId node) noexcept { node_ = node; }

    void emit(Value value) { output_.push_back(std::move(value)); }
    const std::vector<Value>& output() const noexcept { return output_; }

private:
    NodeId node_ = InvalidNodeId;
    std::vector<Value> output_;
};

struct Action {
    std::function<void(ActionContext&, std::span<const Value>)> invoke;
    std::vector<Value> argumentTypes;
};

struct Export {
    NodeId owner = InvalidNodeId;
    std::string name;
    ExportKind kind = ExportKind::Param;
};

struct ResolvedExport {
    void* object = nullptr;
    Value (*get)(const void*) = nullptr;
    void (*set)(void*, const Value&) = nullptr;
    void (*invoke)(void*, ActionContext&, std::span<const Value>) = nullptr;
    std::span<const Value> argumentTypes;

    explicit operator bool() const noexcept {
        return object || get || set || invoke;
    }
};

struct VisibleExport {
    RoleId role = InvalidRoleId;
    NodeId owner = InvalidNodeId;
    ExportId exportId = InvalidExportId;
    ContextScopeId scope = InvalidContextScopeId;
    ContextResolutionState state{};
    bool global = false;
};

class Exports {
    std::vector<Export> exports_;
    std::vector<Param> params_;
    std::vector<Action> actions_;

    std::unordered_map<ExportKey, ExportId, ExportKeyHash> index_;

public:
    template<typename T>
    ExportId param(NodeId owner, std::string_view name, T& value) {
        const ExportKey key{owner, std::string(name)};

        if (index_.contains(key))
            throw Exception("Exports", "Export #{}:'{}' already exists", owner, name);

        const ExportId id = static_cast<ExportId>(exports_.size());

        exports_.push_back({
            .owner = owner,
            .name = std::string(name),
            .kind = ExportKind::Param
        });

        params_.push_back({
            .object = &value,
            .get = [](const void* object) -> Value {
                return Value{*static_cast<const T*>(object)};
            },
            .set = [](void* object, const Value& value) {
                *static_cast<T*>(object) = value.get<T>();
            }
        });

        actions_.push_back({});
        index_.emplace(key, id);

        return id;
    }

    template<typename... Args, typename F>
    ExportId action(NodeId owner, std::string_view name, F&& callback) {
        const ExportKey key{owner, std::string(name)};

        if (index_.contains(key))
            throw Exception("Exports", "Export #{}:'{}' already exists", owner, name);

        const ExportId id = static_cast<ExportId>(exports_.size());

        exports_.push_back({
            .owner = owner,
            .name = std::string(name),
            .kind = ExportKind::Action
        });

        params_.push_back({});
        using Callback = std::decay_t<F>;
        actions_.push_back({
            .invoke = [callback = Callback(std::forward<F>(callback))](
                ActionContext& context,
                std::span<const Value> arguments
            ) mutable {
                if (arguments.size() != sizeof...(Args))
                    throw Exception(
                        "Action",
                        "Expected {} arguments, received {}",
                        sizeof...(Args),
                        arguments.size()
                    );

                invokeCallback<Args...>(callback, context, arguments, std::index_sequence_for<Args...>{});
            },
            .argumentTypes = {argumentType<Args>()...}
        });

        index_.emplace(key, id);

        return id;
    }

    ExportId find(NodeId owner, std::string_view name) const {
        const auto it = index_.find(ExportKey{owner, std::string(name)});
        return it == index_.end() ? InvalidExportId : it->second;
    }

    void alias(NodeId owner, std::string_view name, ExportId target) {
        if (target >= exports_.size() || exports_[target].owner != owner)
            throw Exception("Exports", "Export #{} does not belong to node #{}", target, owner);

        const ExportKey key{owner, std::string(name)};
        if (index_.contains(key))
            throw Exception("Exports", "Export #{}:'{}' already exists", owner, name);

        index_.emplace(std::move(key), target);
    }

    const Export* get(ExportId id) const {
        return id < exports_.size() ? &exports_[id] : nullptr;
    }

    Value value(ExportId id) const {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Param || !params_[id].get)
            throw Exception("Exports", "Export #{} is not readable", id);

        return params_[id].get(params_[id].object);
    }

    template<typename T>
    T value(ExportId id) const {
        return value(id).get<T>();
    }

    void set(ExportId id, const Value& value) {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Param || !params_[id].set)
            throw Exception("Exports", "Export #{} is not writable", id);

        params_[id].set(params_[id].object, value);
    }

    template<typename T>
    void set(ExportId id, T value) {
        set(id, Value{std::move(value)});
    }

    void invoke(
        ExportId id,
        ActionContext& context,
        std::span<const Value> arguments = {}
    ) {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Action || !actions_[id].invoke)
            throw Exception("Exports", "Export #{} is not callable", id);

        actions_[id].invoke(context, arguments);
    }

    void invoke(ExportId id) {
        ActionContext context;
        invoke(id, context);
    }

    ResolvedExport resolve(ExportId id) {
        const auto& entry = exports_.at(id);

        if (entry.kind == ExportKind::Param) {
            auto& param = params_[id];
            return {
                .object = param.object,
                .get = param.get,
                .set = param.set
            };
        }

        auto& action = actions_[id];

        return {
            .object = &action,
            .invoke = [](void* object, ActionContext& context, std::span<const Value> arguments) {
                static_cast<Action*>(object)->invoke(context, arguments);
            },
            .argumentTypes = action.argumentTypes
        };
    }

    void remove(NodeId owner) {
        std::erase_if(index_, [owner](const auto& item) {
            return item.first.owner == owner;
        });

        for (ExportId id = 0; id < exports_.size(); ++id) {
            if (exports_[id].owner != owner)
                continue;

            exports_[id].owner = InvalidNodeId;
            params_[id] = {};
            actions_[id] = {};
        }
    }

    void dump() const {
        Logger::Tree tree("Exports");

        for (ExportId id = 0; id < exports_.size(); ++id) {
            const auto& entry = exports_[id];
            if (entry.owner == InvalidNodeId) continue;

            std::string value;

            if (entry.kind == ExportKind::Param) {
                const auto& param = params_[id];
                value = param.get ? param.get(param.object).toString() : "<unreadable>";
            }

            const std::string_view kind = entry.kind == ExportKind::Param ? "<g>@</>" : "<y>λ</>";
            tree.node(std::format("{}{}<gr> = {}</>", kind, entry.name, value), 0);
        }

        tree.print();
    }

private:
    template<typename>
    static constexpr bool AlwaysFalse = false;

    template<typename T>
    static Value argumentType() {
        using U = std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<U, std::string>)
            return Value{std::string{}};
        else if constexpr (std::is_same_v<U, bool>)
            return Value{false};
        else if constexpr (std::is_integral_v<U>)
            return Value{int64_t{0}};
        else if constexpr (std::is_floating_point_v<U>)
            return Value{0.0};
        else if constexpr (
            std::is_same_v<U, glm::vec2> ||
            std::is_same_v<U, glm::vec3> ||
            std::is_same_v<U, glm::vec4>
        )
            return Value{U{}};
        else
            static_assert(AlwaysFalse<U>, "Unsupported action argument type");
    }

    template<typename T>
    static std::remove_cvref_t<T> argument(const Value& value) {
        return value.as<std::remove_cvref_t<T>>();
    }

    template<typename... Args, typename F, size_t... Indices>
    static void invokeCallback(
        F& callback,
        ActionContext& context,
        std::span<const Value> arguments,
        std::index_sequence<Indices...>
    ) {
        if constexpr (std::is_invocable_v<F&, ActionContext&, std::remove_cvref_t<Args>...>)
            std::invoke(callback, context, argument<Args>(arguments[Indices])...);
        else if constexpr (std::is_invocable_v<F&, std::remove_cvref_t<Args>...>)
            std::invoke(callback, argument<Args>(arguments[Indices])...);
        else
            static_assert(AlwaysFalse<F>, "Action callback does not match its declared arguments");
    }
};

}
