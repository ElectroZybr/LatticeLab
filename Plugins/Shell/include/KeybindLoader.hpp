#pragma once

#include <string>
#include <string_view>
#include <utility>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Value.hpp>

#include "ActionMap.hpp"
#include "LoaderAPI.hpp"

class KeybindsLoader final : public LoaderAPI {
    static constexpr std::string_view tag = "KeybindsLoader";

public:
    void configure(Lattice::Node& branch) {
        Lattice::Context& ctx = branch.requireContext();
        const Lattice::ObjectId id = ctx.find(Lattice::typeName<ActionMap>());
        if (!Lattice::Objects::valid(id))
            throw Lattice::Exception(tag, "ActionMap is not active in context");

        actionMap = static_cast<ActionMap*>(ctx.objects.require(id).node->getObject());
    }

    std::string_view section() const override { return "keybinds"; }

    void load(const Lattice::Value& section) override {
        if (!section.is<Lattice::Table>())
            return;

        if (!actionMap)
            throw Lattice::Exception(tag, "loader is not configured");

        loadTable(std::get<Lattice::Table>(section), "");
    }

private:
    Ref<ActionMap> actionMap;

    static bool isOp(std::string_view name) {
        return name == "add" || name == "sub" || name == "toggle";
    }

    static bool asNumber(const Lattice::Value& value, double& out) {
        if (value.is<double>()) {
            out = value.get<double>();
            return true;
        }

        if (value.is<int64_t>()) {
            out = static_cast<double>(value.get<int64_t>());
            return true;
        }

        return false;
    }

    static std::pair<std::string, std::string> splitVerb(std::string path) {
        const auto pos = path.rfind('.');
        if (pos == std::string::npos || pos + 1 >= path.size())
            return {std::move(path), "action"};

        std::string last = path.substr(pos + 1);
        if (!isOp(last))
            return {std::move(path), "action"};

        path.resize(pos);
        return {std::move(path), std::move(last)};
    }

    void loadTable(const Lattice::Table& table, const std::string& prefix) {
        for (const auto& [key, value] : table) {
            const std::string path = prefix.empty() ? key : prefix + "." + key;

            if (value.is<Lattice::Table>()) {
                loadTable(std::get<Lattice::Table>(value), path);
                continue;
            }

            bindEntry(path, value);
        }
    }

    void bindEntry(std::string path, const Lattice::Value& value) {
        auto [verb, op] = splitVerb(std::move(path));
        if (verb.empty())
            return;

        Lattice::Array args;
        if (value.is<Lattice::Array>())
            args = std::get<Lattice::Array>(value);
        else
            args.push_back(value);

        if (args.empty() || !args[0].is<std::string>())
            throw Lattice::Exception(tag, "bind '{}' needs a trigger string", verb);

        const auto& trigger = std::get<std::string>(args[0]);
        ActionMode mode = ActionMode::OnPress;
        double delta = 0.0;

        for (size_t i = 1; i < args.size(); ++i) {
            const auto& arg = args[i];

            if (arg.is<std::string>()) {
                const auto& token = std::get<std::string>(arg);

                if (isOp(token))
                    op = token;
                else if (token == "hold")
                    mode = ActionMode::OnHold;
                else if (token == "press")
                    mode = ActionMode::OnPress;
                else if (token == "release")
                    mode = ActionMode::OnRelease;
            } else {
                asNumber(arg, delta);
            }
        }

        if (op == "toggle")
            actionMap->bindToggle(verb, trigger, mode);
        else if (op == "add" || op == "sub")
            actionMap->bindAdd(verb, trigger, delta, mode);
        else
            actionMap->bind(verb, trigger, mode);
    }
};