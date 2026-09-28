#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/Value.hpp>

#include "ActionRouter.hpp"
#include "LoaderAPI.hpp"

class KeybindsLoader final : public LoaderAPI {
    static constexpr std::string_view tag = "KeybindsLoader";
    Focus<ActionRouter> actionMap_;

public:
    void configure(NodeConfigure node) {
        node.focus<ActionRouter>();
    }

    std::string_view section() const override { return "keybinds"; }

    void load(const Lattice::Value& section) override {
        if (!section.is<Lattice::Object>())
            return;

        if (!actionMap_)
            throw Lattice::Exception(tag, "loader is not configured");

        loadTable(section.require<Lattice::Object>(), "");
    }

private:
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

    void loadTable(const Lattice::Object& table, const std::string& prefix) {
        for (const auto& [key, value] : table) {
            const std::string path = prefix.empty() ? key : prefix + "." + key;

            if (value.is<Lattice::Object>()) {
                loadTable(value.require<Lattice::Object>(), path);
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
            args = value.get<Lattice::Array>();
        else
            args.push_back(value);

        if (args.empty() || !args[0].is<std::string>())
            throw Lattice::Exception(tag, "bind '{}' needs a trigger string", verb);

        const auto trigger = args[0].get<std::string>();
        ActionMode mode = ActionMode::OnPress;
        double delta = 0.0;

        for (size_t i = 1; i < args.size(); ++i) {
            const auto& arg = args[i];

            if (arg.is<std::string>()) {
                const auto token = arg.get<std::string>();

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
            actionMap_->bindToggle(verb, trigger, mode);
        else if (op == "add" || op == "sub")
            actionMap_->bindAdd(verb, trigger, op == "sub" ? -delta : delta, mode);
        else
            actionMap_->bind(verb, trigger, mode);
    }
};