#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/SubsystemAPI.hpp>
#include "InputAPI.hpp"

namespace Lattice { class Node; }

enum class ActionMode { OnPress, OnHold, OnRelease };
enum class Target { Action, Toggle, Add };

class ActionMap final : public SubsystemAPI {
public:
    explicit ActionMap(Lattice::Node& branch) {}
    void configure(Lattice::Node& branch);

    void bind(std::string_view verb, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindToggle(std::string_view param, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindAdd(std::string_view param, std::string_view trigger, double delta, ActionMode mode = ActionMode::OnPress);

    void tick();

    bool down(std::string_view verb) const;
    bool pressed(std::string_view verb) const;
    bool released(std::string_view verb) const;

    void clearBinds();

private:
    struct Binding {
        std::string verb;
        std::string trigger;
        ActionMode mode = ActionMode::OnPress;
        Target target = Target::Action;
        double delta = 0;
        bool wasDown = false;
    };

    struct ActionState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    Lattice::Node* node_ = nullptr;
    std::vector<InputAPI*> inputs_;
    std::vector<Binding> bindings_;
    std::unordered_map<std::string, ActionState> actions_;

    ActionState& ensure(std::string_view verb);
    const ActionState* find(std::string_view verb) const;
};