#include "ActionMap.hpp"

#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/Logger.hpp>


void ActionMap::configure(Lattice::Node& branch) {
    run_ctx = &branch.requireContext();
    inputs_ = branch.globalCollect<InputAPI>();
}

ActionMap::ActionState& ActionMap::ensure(Lattice::SlotId slot) {
    return actions_[slot];
}

const ActionMap::ActionState* ActionMap::find(Lattice::SlotId slot) const {
    auto it = actions_.find(slot);
    return it == actions_.end() ? nullptr : &it->second;
}

Lattice::SlotId ActionMap::resolve(std::string_view verb) const {
    return run_ctx->getSlot(verb);
}

void ActionMap::bind(std::string_view verb, std::string_view trigger, ActionMode mode) {
    const auto slot = resolve(verb);

    ensure(slot);
    bindings_.push_back({slot, std::string(trigger), mode});

    Logger::ok("ActionMap", "bound '{}' ➜ '{}'", trigger, verb);
}

void ActionMap::bindToggle(std::string_view param, std::string_view trigger, ActionMode mode) {
    const auto slot = resolve(param);

    ensure(slot);
    bindings_.push_back({
        slot,
        std::string(trigger),
        mode,
        Target::Toggle
    });

    Logger::ok("ActionMap", "bound toggle '{}' ➜ '{}'", trigger, param);
}

void ActionMap::bindAdd(std::string_view param, std::string_view trigger, double delta, ActionMode mode) {
    const auto slot = resolve(param);

    ensure(slot);
    bindings_.push_back({
        slot,
        std::string(trigger),
        mode,
        Target::Add,
        delta
    });

    Logger::ok("ActionMap", "bound add '{}' ➜ '{}' ({})", trigger, param, delta);
}

void ActionMap::tick() {
    for (auto& [_, state] : actions_)
        state = {};

    for (auto& b : bindings_) {
        bool now = false;

        for (auto* input : inputs_) {
            if (input && input->down(b.trigger)) {
                now = true;
                break;
            }
        }

        const bool pressed  = now && !b.wasDown;
        const bool released = !now && b.wasDown;

        auto& state = ensure(b.slot);

        state.down |= now;
        state.pressed |= pressed;
        state.released |= released;

        const bool fire =
            (b.mode == ActionMode::OnPress   && pressed) ||
            (b.mode == ActionMode::OnHold    && now) ||
            (b.mode == ActionMode::OnRelease && released);

        if (fire) {
            Logger::info("ActionMap", "fire from: {}", b.trigger);
            if (b.target == Target::Action) {
                run_ctx->invoke(b.slot);
            } else if (b.target == Target::Toggle) {
                run_ctx->set(b.slot, !run_ctx->getValue<bool>(b.slot));
            } else {
                run_ctx->set(b.slot, run_ctx->getValue<double>(b.slot) + b.delta);
            }
        }

        b.wasDown = now;
    }
}

bool ActionMap::down(Lattice::SlotId slot) const {
    const auto* state = find(slot);
    return state && state->down;
}

bool ActionMap::pressed(Lattice::SlotId slot) const {
    const auto* state = find(slot);
    return state && state->pressed;
}

bool ActionMap::released(Lattice::SlotId slot) const {
    const auto* state = find(slot);
    return state && state->released;
}

void ActionMap::clearBinds() {
    bindings_.clear();

    for (auto& [_, state] : actions_)
        state = {};
}