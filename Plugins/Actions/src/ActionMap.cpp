#include "ActionMap.hpp"

#include <utility>

#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "Lattice/Kernel/Context.hpp"
#include "Lattice/Kernel/Objects.hpp"


void ActionMap::configure(Lattice::Node& branch) {
    run_ctx = &branch.requireContext();
    inputs_ = branch.globalCollect<InputAPI>();
}

ActionMap::ActionState& ActionMap::ensure(Lattice::ContextId slot) {
    return actions_[slot];
}

const ActionMap::ActionState* ActionMap::find(Lattice::ContextId slot) const {
    auto it = actions_.find(slot);
    return it == actions_.end() ? nullptr : &it->second;
}

ActionMap::Binding* ActionMap::findBind(Lattice::ContextId slot, std::string_view trigger) {
    return const_cast<Binding*>(std::as_const(*this).findBind(slot, trigger));
}

const ActionMap::Binding* ActionMap::findBind(Lattice::ContextId slot, std::string_view trigger) const {
    for (const auto& binding : bindings_) {
        if (binding.slot == slot && binding.trigger == trigger)
            return &binding;
    }
    return nullptr;
}

bool ActionMap::hasBind(std::string_view verb, std::string_view trigger) const {
    if (!run_ctx)
        return false;

    const Lattice::ContextId slot = run_ctx->contexts.find(verb);
    if (!Lattice::ContextRegistry::valid(slot))
        return false;

    return findBind(slot, trigger) != nullptr;
}

void ActionMap::upsert(
    std::string_view verb,
    std::string_view trigger,
    ActionMode mode,
    Target target,
    double delta
) {
    const Lattice::ContextId slot = run_ctx->getOrCreate(verb);
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
            const Lattice::ObjectId object = run_ctx->get(b.slot);

            if (Lattice::Objects::valid(object)) {
                Logger::info("ActionMap", "fire from: {}", b.trigger);

                if (b.target == Target::Action) {
                    run_ctx->bindings.invoke(object);
                } else if (b.target == Target::Toggle) {
                    run_ctx->bindings.set(object, !run_ctx->bindings.get<bool>(object));
                } else {
                    run_ctx->bindings.set(object, run_ctx->bindings.get<double>(object) + b.delta);
                }
            }
        }

        b.wasDown = now;
    }
}

bool ActionMap::down(Lattice::ContextId slot) const {
    const auto* state = find(slot);
    return state && state->down;
}

bool ActionMap::pressed(Lattice::ContextId slot) const {
    const auto* state = find(slot);
    return state && state->pressed;
}

bool ActionMap::released(Lattice::ContextId slot) const {
    const auto* state = find(slot);
    return state && state->released;
}

void ActionMap::clearBinds() {
    bindings_.clear();

    for (auto& [_, state] : actions_)
        state = {};
}