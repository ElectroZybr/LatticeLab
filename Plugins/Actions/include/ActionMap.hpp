#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/SubsystemAPI.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/RefSlot.hpp>
#include "Lattice/Kernel/Objects.hpp"

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

    bool down(Lattice::ContextId slot) const;
    bool pressed(Lattice::ContextId slot) const;
    bool released(Lattice::ContextId slot) const;

    size_t bindCount() const { return bindings_.size(); }
    bool hasBind(std::string_view verb, std::string_view trigger) const;

    void clearBinds();
    Ref<Lattice::Context> run_ctx;

private:
    struct Binding {
        Lattice::ContextId slot = Lattice::InvalidContextId;
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

    std::vector<InputAPI*> inputs_;

    std::vector<Binding> bindings_;
    std::unordered_map<Lattice::ContextId, ActionState> actions_;

    ActionState& ensure(Lattice::ContextId slot);
    const ActionState* find(Lattice::ContextId slot) const;

    Binding* findBind(Lattice::ContextId slot, std::string_view trigger);
    const Binding* findBind(Lattice::ContextId slot, std::string_view trigger) const;
    void upsert(
        std::string_view verb,
        std::string_view trigger,
        ActionMode mode,
        Target target,
        double delta
    );
};