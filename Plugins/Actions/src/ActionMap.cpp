#include "ActionMap.hpp"

#include <format>
#include <utility>

#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Tools/Logger.hpp>


std::string ActionMap::bindName(Lattice::RoleId role, std::string_view trigger) {
    return std::format("{}:{}", role, trigger);
}

void ActionMap::configure(Lattice::Node& branch) {
    run_ctx = &branch.requireContext();
    inputs_ = branch.collect<InputAPI>();
}

ActionMap::Binding* ActionMap::findBind(Lattice::RoleId role, std::string_view trigger) {
    return const_cast<Binding*>(std::as_const(*this).findBind(role, trigger));
}

const ActionMap::Binding* ActionMap::findBind(Lattice::RoleId role, std::string_view trigger) const {
    return bindings_.get(bindings_.find(bindName(role, trigger)));
}

bool ActionMap::any(Lattice::RoleId role, bool Binding::* field) const {
    for (BindId id = 0; id < bindings_.size(); ++id) {
        const Binding* binding = bindings_.get(id);
        if (binding && binding->role == role && binding->*field)
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

    const Lattice::RoleId role = run_ctx->roles.find(verb);
    if (!Lattice::RoleRegistry::valid(role))
        return false;

    return findBind(role, trigger) != nullptr;
}

void ActionMap::upsert(std::string_view verb, std::string_view trigger, ActionMode mode, Target target, double delta) {
    const Lattice::RoleId role = run_ctx->getOrCreateRole(verb);

    if (Binding* existing = findBind(role, trigger)) {
        existing->mode = mode;
        existing->target = target;
        existing->delta = delta;
        existing->wasDown = false;
        Logger::info("ActionMap", "rebound '{}' ➜ '{}'", trigger, verb);
        return;
    }

    bindings_.create({
        .name = bindName(role, trigger),
        .role = role,
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

void ActionMap::bindAxis2(std::string_view param, std::string_view trigger) {
    const Lattice::RoleId role = run_ctx->getOrCreateRole(param);
    bindings_.create({
        .name = bindName(role, trigger),
        .role = role,
        .trigger = std::string(trigger),
        .input = InputKind::Axis2,
    });
}

void ActionMap::tick() {
    for (BindId id = 0; id < bindings_.size(); ++id) {
        Binding* binding = bindings_.get(id);
        if (!binding) continue;

        binding->down = false;
        binding->pressed = false;
        binding->released = false;

        const Lattice::ObjectId object = run_ctx->resolveFocus(Lattice::InvalidFocusScopeId, binding->role);
        if (object == Lattice::InvalidObjectId) continue;

        if (binding->input == InputKind::Axis) {
            double value = 0.0;
            for (auto* input : inputs_) if (input) value += input->axis(binding->trigger);
            run_ctx->bindings.set(object, value);
            continue;
        }

        if (binding->input == InputKind::Axis2) {
            glm::vec2 value{};
            for (auto* input : inputs_) if (input) value += input->axis2(binding->trigger);
            run_ctx->bindings.set(object, value);
            continue;
        }

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
            Logger::info("ActionMap", "fire from: {}", binding->trigger);
            switch (binding->target) {
                case Target::Action:
                    run_ctx->bindings.invoke(object);
                    break;
                case Target::Toggle:
                    run_ctx->bindings.set(object, !run_ctx->bindings.get<bool>(object));
                    break;
                case Target::Add:
                    run_ctx->bindings.set(object, run_ctx->bindings.get<double>(object) + binding->delta);
                    break;
            }
        }

        binding->wasDown = now;
    }
}

bool ActionMap::down(Lattice::RoleId role) const {
    return any(role, &Binding::down);
}

bool ActionMap::pressed(Lattice::RoleId role) const {
    return any(role, &Binding::pressed);
}

bool ActionMap::released(Lattice::RoleId role) const {
    return any(role, &Binding::released);
}

void ActionMap::clearBinds() {
    bindings_.clear();
}
