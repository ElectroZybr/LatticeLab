#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Value.hpp>
#include "Lattice/Tools/LogTree.hpp"

namespace Lattice {

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

struct Action {
    std::function<void()> invoke;
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
    void (*invoke)(void*) = nullptr;

    explicit operator bool() const noexcept {
        return object || get || set || invoke;
    }
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

    template<typename F>
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
        actions_.push_back({
            .invoke = std::forward<F>(callback)
        });

        index_.emplace(key, id);

        return id;
    }

    ExportId find(NodeId owner, std::string_view name) const {
        const auto it = index_.find(ExportKey{owner, std::string(name)});
        return it == index_.end() ? InvalidExportId : it->second;
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

    void invoke(ExportId id) {
        if (id >= exports_.size() || exports_[id].kind != ExportKind::Action || !actions_[id].invoke)
            throw Exception("Exports", "Export #{} is not callable", id);

        actions_[id].invoke();
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
            .invoke = [](void* object) {
                static_cast<Action*>(object)->invoke();
            }
        };
    }

    void remove(NodeId owner) {
        for (ExportId id = 0; id < exports_.size(); ++id) {
            if (exports_[id].owner != owner)
                continue;

            index_.erase(ExportKey{owner, exports_[id].name});
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
};

}