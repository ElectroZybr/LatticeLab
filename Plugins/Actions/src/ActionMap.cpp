#include "ActionMap.hpp"

#include <utility>

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

ActionMap::Binding* ActionMap::findBind(Lattice::SlotId slot, std::string_view trigger) {
    return const_cast<Binding*>(std::as_const(*this).findBind(slot, trigger));
}

const ActionMap::Binding* ActionMap::findBind(Lattice::SlotId slot, std::string_view trigger) const {
    for (const auto& binding : bindings_) {
        if (binding.slot == slot && binding.trigger == trigger)
            return &binding;
    }

    return nullptr;
}

void ActionMap::upsert(
    std::string_view verb,
    std::string_view trigger,
    ActionMode mode,
    Target target,
    double delta
) {
    const auto slot = resolve(verb);
    ensure(slot);

    if (auto* existing = findBind(slot, trigger)) {
        existing->mode = mode;
        existing->target = target;
        existing->delta = delta;
        existing->wasDown = false;
        Logger::info("ActionMap", "rebound '{}' ➜ '{}'", trigger, verb);
        return;
    }

    bindings_.push_back({
        slot,
        std::string(trigger),
        mode,
        target,
        delta
    });

    if (target == Target::Toggle)
        Logger::ok("ActionMap", "bound toggle '{}' ➜ '{}'", trigger, verb);
    else if (target == Target::Add)
        Logger::ok("ActionMap", "bound add '{}' ➜ '{}' ({})", trigger, verb, delta);
    else
        Logger::ok("ActionMap", "bound '{}' ➜ '{}'", trigger, verb);
}

void ActionMap::bind(std::string_view verb, std::string_view trigger, ActionMode mode) {
    upsert(verb, trigger, mode, Target::Action, 0);
}

void ActionMap::bindToggle(std::string_view param, std::string_view trigger, ActionMode mode) {
    upsert(param, trigger, mode, Target::Toggle, 0);
}

void ActionMap::bindAdd(std::string_view param, std::string_view trigger, double delta, ActionMode mode) {
    upsert(param, trigger, mode, Target::Add, delta);
}

bool ActionMap::hasBind(std::string_view verb, std::string_view trigger) const {
    if (!run_ctx)
        return false;

    const auto slot = run_ctx->findSlot(verb);
    if (slot == Lattice::InvalidSlotId)
        return false;

    return findBind(slot, trigger) != nullptr;
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

            const Lattice::ObjectId object = run_ctx->get(b.slot);

            if (b.target == Target::Action) {
                run_ctx->bindings.invoke(object);
            } else if (b.target == Target::Toggle) {
                run_ctx->bindings.set(object, !run_ctx->bindings.get<bool>(object));
            } else {
                run_ctx->bindings.set(object, run_ctx->bindings.get<double>(object) + b.delta);
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