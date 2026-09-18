#include "ActionMap.hpp"

#include <format>
#include <utility>

#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Tools/Logger.hpp>


std::string ActionMap::bindName(Lattice::ContextId slot, std::string_view trigger) {
    return std::format("{}:{}", slot, trigger);
}

void ActionMap::configure(Lattice::Node& branch) {
    run_ctx = &branch.requireContext();
    inputs_ = branch.collect<InputAPI>();
}

ActionMap::Binding* ActionMap::findBind(Lattice::ContextId slot, std::string_view trigger) {
    return const_cast<Binding*>(std::as_const(*this).findBind(slot, trigger));
}

const ActionMap::Binding* ActionMap::findBind(Lattice::ContextId slot, std::string_view trigger) const {
    return bindings_.get(bindings_.find(bindName(slot, trigger)));
}

bool ActionMap::any(Lattice::ContextId slot, bool Binding::* field) const {
    for (BindId id = 0; id < bindings_.size(); ++id) {
        const Binding* binding = bindings_.get(id);
        if (binding && binding->slot == slot && binding->*field)
            return true;
    }
    return false;
}

size_t ActionMap::bindCount() const {
    size_t count = 0;
    for (BindId id = 0; id < bindings_.size(); ++id) {
        if (bindings_.get(id))
            ++count;
    }
    return count;
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

    if (Binding* existing = findBind(slot, trigger)) {
        existing->mode = mode;
        existing->target = target;
        existing->delta = delta;
        existing->wasDown = false;
        Logger::info("ActionMap", "rebound '{}' ➜ '{}'", trigger, verb);
        return;
    }

    bindings_.create({
        .name = bindName(slot, trigger),
        .slot = slot,
        .trigger = std::string(trigger),
        .mode = mode,
        .target = target,
        .delta = delta
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
    for (BindId id = 0; id < bindings_.size(); ++id) {
        Binding* binding = bindings_.get(id);
        if (!binding)
            continue;

        binding->down = false;
        binding->pressed = false;
        binding->released = false;
    }

    for (BindId id = 0; id < bindings_.size(); ++id) {
        Binding* binding = bindings_.get(id);
        if (!binding)
            continue;

        bool now = false;
        for (auto* input : inputs_) {
            if (input && input->down(binding->trigger)) {
                now = true;
                break;
            }
        }

        const bool pressed = now && !binding->wasDown;
        const bool released = !now && binding->wasDown;

        binding->down = now;
        binding->pressed = pressed;
        binding->released = released;

        const bool fire =
            (binding->mode == ActionMode::OnPress && pressed) ||
            (binding->mode == ActionMode::OnHold && now) ||
            (binding->mode == ActionMode::OnRelease && released);

        if (fire) {
            const Lattice::ObjectId object = run_ctx->get(binding->slot);

            if (object != Lattice::InvalidObjectId) {
                Logger::info("ActionMap", "fire from: {}", binding->trigger);

                if (binding->target == Target::Action) {
                    run_ctx->bindings.invoke(object);
                } else if (binding->target == Target::Toggle) {
                    run_ctx->bindings.set(object, !run_ctx->bindings.get<bool>(object));
                } else {
                    run_ctx->bindings.set(
                        object,
                        run_ctx->bindings.get<double>(object) + binding->delta
                    );
                }
            }
        }

        binding->wasDown = now;
    }
}

bool ActionMap::down(Lattice::ContextId slot) const {
    return any(slot, &Binding::down);
}

bool ActionMap::pressed(Lattice::ContextId slot) const {
    return any(slot, &Binding::pressed);
}

bool ActionMap::released(Lattice::ContextId slot) const {
    return any(slot, &Binding::released);
}

void ActionMap::clearBinds() {
    bindings_.clear();
}
