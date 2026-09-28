#pragma once

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/TypeName.hpp>
#include <Lattice/Kernel/Value.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "Lattice/Tools/TreeFormatter.hpp"

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

/**
 * Non-owning reference to a structured action result.
 *
 * The referenced object must remain alive until the caller has consumed the
 * ActionContext output.
 */
class ActionView {
public:
    template<typename T>
    explicit ActionView(T& object) noexcept
        : object_(&object), type_(typeKey<std::remove_cvref_t<T>>()) {}

    std::string_view type() const noexcept { return type_; }

    template<typename T>
    bool is() const noexcept {
        return type_ == typeKey<std::remove_cvref_t<T>>();
    }

    template<typename T>
    const std::remove_cvref_t<T>& as() const {
        using View = std::remove_cvref_t<T>;
        if (!is<View>())
            throw Exception<ActionView>(
                "View contains '{}', requested '{}'",
                type_,
                typeKey<View>()
            );
        return *static_cast<const View*>(object_);
    }

private:
    const void* object_ = nullptr;
    std::string_view type_;
};

using ActionOutput = std::variant<Value, ActionView>;

class ActionContext {
public:
    explicit ActionContext(NodeId node = InvalidNodeId) : node_(node) {}

    NodeId node() const noexcept { return node_; }
    void setNode(NodeId node) noexcept { node_ = node; }

    void emit(Value value) { output_.emplace_back(std::move(value)); }

    template<typename T>
    void present(const T& view) {
        output_.emplace_back(std::in_place_type<ActionView>, view);
    }

    template<typename T>
    void present(const T&&) = delete;

    std::span<const ActionOutput> output() const noexcept { return output_; }

private:
    NodeId node_ = InvalidNodeId;
    std::vector<ActionOutput> output_;
};

struct Action {
    std::function<void(ActionContext&, std::span<const Value>)> invoke;
    std::vector<Value> argumentTypes;
    size_t requiredArguments = 0;
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
    size_t requiredArguments = 0;

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
            throw Exception<Exports>("Export #{}:'{}' already exists", owner, name);

        const ExportId id = static_cast<ExportId>(exports_.size());

        exports_.push_back({
            .owner = owner,
            .name = std::string(name),
            .kind = ExportKind::Param
        });

        params_.push_back({
            .object = &value,
            .get = [](const void* object) -> Value {
                return ParamAdapter<T>::get(*static_cast<const T*>(object));
            },
            .set = [](void* object, const Value& value) {
                ParamAdapter<T>::set(*static_cast<T*>(object), value);
            }
        });

        actions_.push_back({});
        index_.emplace(key, id);

        return id;
    }

    template<typename... Args, typename F>
    ExportId action(NodeId owner, std::string_view name, F&& callback) {
        static_assert(
            optionalArgumentsAreTrailing<Args...>(),
            "Optional action arguments must follow required arguments"
        );

        const ExportKey key{owner, std::string(name)};

        if (index_.contains(key))
            throw Exception<Exports>("Export #{}:'{}' already exists", owner, name);

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
                constexpr size_t required = requiredArgumentCount<Args...>();
                if (arguments.size() < required || arguments.size() > sizeof...(Args)) {
                    if constexpr (required == sizeof...(Args)) {
                        throw Exception<Action>(
                            "Expected {} arguments, received {}",
                            required,
                            arguments.size()
                        );
                    } else {
                        throw Exception<Action>(
                            "Expected {} to {} arguments, received {}",
                            required,
                            sizeof...(Args),
                            arguments.size()
                        );
                    }
                }

                invokeCallback<Args...>(callback, context, arguments, std::index_sequence_for<Args...>{});
            },
            .argumentTypes = {argumentType<Args>()...},
            .requiredArguments = requiredArgumentCount<Args...>()
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
            throw Exception<Exports>("Export #{} does not belong to node #{}", target, owner);

        const ExportKey key{owner, std::string(name)};
        if (index_.contains(key))
            throw Exception<Exports>("Export #{}:'{}' already exists", owner, name);

        index_.emplace(std::move(key), target);
    }

    const Export* get(ExportId id) const {
        return id < exports_.size() ? &exports_[id] : nullptr;
    }

    Value value(ExportId id) const {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Param || !params_[id].get)
            throw Exception<Exports>("Export #{} is not readable", id);

        return params_[id].get(params_[id].object);
    }

    template<typename T>
    T value(ExportId id) const {
        return value(id).get<T>();
    }

    void set(ExportId id, const Value& value) {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Param || !params_[id].set)
            throw Exception<Exports>("Export #{} is not writable", id);

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
            throw Exception<Exports>("Export #{} is not callable", id);

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
            .argumentTypes = action.argumentTypes,
            .requiredArguments = action.requiredArguments
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
        Lattice::TreeFormatter tree("Exports");

        for (ExportId id = 0; id < exports_.size(); ++id) {
            const auto& entry = exports_[id];
            if (entry.owner == InvalidNodeId) continue;

            std::string value;

            if (entry.kind == ExportKind::Param) {
                const auto& param = params_[id];
                value = param.get ? param.get(param.object).toString() : "<unreadable>";
            }

            const std::string_view kind = entry.kind == ExportKind::Param ? "<ok>@</>" : "<wrn>λ</>";
            tree.node(std::format("{}{}<mut2> = {}</>", kind, entry.name, value), 0);
        }

        Logger::message(tree.format());
        Logger::blank();
    }

private:
    template<typename>
    static constexpr bool AlwaysFalse = false;

    template<typename T>
    struct OptionalArgument : std::false_type {
        using Type = std::remove_cvref_t<T>;
    };

    template<typename T>
    struct OptionalArgument<std::optional<T>> : std::true_type {
        using Type = T;
    };

    template<typename T>
    static constexpr bool IsOptionalArgument =
        OptionalArgument<std::remove_cvref_t<T>>::value;

    template<typename... Args>
    static consteval bool optionalArgumentsAreTrailing() {
        bool optionalSeen = false;
        bool valid = true;
        ((IsOptionalArgument<Args>
            ? optionalSeen = true
            : valid = valid && !optionalSeen), ...);
        return valid;
    }

    template<typename... Args>
    static consteval size_t requiredArgumentCount() {
        return (size_t{0} + ... + (IsOptionalArgument<Args> ? 0 : 1));
    }

    template<typename T>
    static Value argumentType() {
        using U = typename OptionalArgument<std::remove_cvref_t<T>>::Type;

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
    static std::remove_cvref_t<T> argument(
        std::span<const Value> arguments,
        size_t index
    ) {
        using U = std::remove_cvref_t<T>;

        if constexpr (IsOptionalArgument<U>) {
            using ValueType = typename OptionalArgument<U>::Type;
            if (index >= arguments.size())
                return std::nullopt;
            return U{arguments[index].as<ValueType>()};
        } else {
            return arguments[index].as<U>();
        }
    }

    template<typename... Args, typename F, size_t... Indices>
    static void invokeCallback(
        F& callback,
        ActionContext& context,
        std::span<const Value> arguments,
        std::index_sequence<Indices...>
    ) {
        if constexpr (std::is_invocable_v<F&, ActionContext&, std::remove_cvref_t<Args>...>)
            std::invoke(callback, context, argument<Args>(arguments, Indices)...);
        else if constexpr (std::is_invocable_v<F&, std::remove_cvref_t<Args>...>)
            std::invoke(callback, argument<Args>(arguments, Indices)...);
        else
            static_assert(AlwaysFalse<F>, "Action callback does not match its declared arguments");
    }
};

}
