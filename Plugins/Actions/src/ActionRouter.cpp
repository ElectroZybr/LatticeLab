#include "ActionRouter.hpp"

#include <format>

#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Tools/Logger.hpp>


void ActionRouter::configure(Lattice::Node& branch) {
    run_ctx = &branch.requireContext();
    inputs_ = branch.collect<InputAPI>();
    for (auto* input : inputs_)
        if (input) input->registerTriggers(triggers_);
}

std::optional<ActionRouter::TriggerChain> ActionRouter::parseTriggerChain(std::string_view expression) {
    TriggerChain chain;
    size_t begin = 0;

    while (begin < expression.size()) {
        const size_t end = expression.find('+', begin);
        const auto token = expression.substr(begin, end == std::string_view::npos ? expression.size() - begin : end - begin);

        if (token.empty()) {
            Logger::warning("ActionRouter", "invalid trigger chain '{}'", expression);
            return std::nullopt;
        }

        const TriggerId id = triggers_.find(token);
        if (!TriggerRegistry::valid(id)) {
            Logger::warning("ActionRouter", "unknown trigger '{}'", token);
            return std::nullopt;
        }

        if (chain.source == InvalidTriggerId)
            chain.source = id;
        else
            chain.modifiers.push_back(id);

        if (end == std::string_view::npos)
            break;

        begin = end + 1;
    }

    // сортируем цепочку
    std::sort(chain.modifiers.begin(), chain.modifiers.end());
    chain.modifiers.erase(std::unique(chain.modifiers.begin(), chain.modifiers.end()), chain.modifiers.end());

    return chain;
}

bool ActionRouter::any(Lattice::RoleId role, BindingFlags flag) const {
    for (BindId id = 0; id < bindings_.size(); ++id) {
        const Binding* binding = bindings_.get(id);
        if (binding && binding->role == role && hasFlag(*binding, flag))
            return true;
    }
    return false;
}

void ActionRouter::upsert(std::string_view verb, std::string_view expression, ActionMode mode, Target target, double delta) {
    const auto role = run_ctx->getOrCreateRole(verb);
    const auto chain = parseTriggerChain(expression);
    if (!chain) return;
    const auto key = bindKey(role, *chain);

    if (auto* existing = bindings_.get(bindings_.find(key))) {
        existing->mode = mode;
        existing->target = target;
        existing->delta = delta;
        existing->flags = 0;
        return;
    }

    bindings_.create({
        .role = role,
        .trigger = *chain,
        .mode = mode,
        .target = target,
        .delta = delta
    }, key);

    Logger::info("ActionRouter", "bound '{}' ➜ '{}'", expression, verb);
}

void ActionRouter::bind(std::string_view verb, std::string_view trigger, ActionMode mode) {
    upsert(verb, trigger, mode, Target::Action, 0);
}

void ActionRouter::bindToggle(std::string_view verb, std::string_view trigger, ActionMode mode) {
    upsert(verb, trigger, mode, Target::Toggle, 0);
}

void ActionRouter::bindAdd(std::string_view verb, std::string_view trigger, double delta, ActionMode mode) {
    upsert(verb, trigger, mode, Target::Add, delta);
}

void ActionRouter::bindAxis(std::string_view verb, std::string_view expression) {
    const auto role = run_ctx->getOrCreateRole(verb);
    const auto chain = parseTriggerChain(expression);
    if (!chain) return;
    const auto key = bindKey(role, *chain);

    if (auto* existing = bindings_.get(bindings_.find(key))) {
        existing->flags = 0;
        return;
    }

    bindings_.create({
        .role = role,
        .trigger = *chain
    }, key);

    Logger::info("ActionRouter", "bound '{}' ➜ '{}'", expression, verb);
}

void ActionRouter::bindAxis2(std::string_view verb, std::string_view expression) {
    const auto role = run_ctx->getOrCreateRole(verb);
    const auto chain = parseTriggerChain(expression);
    if (!chain) return;
    const auto key = bindKey(role, *chain);

    if (auto* existing = bindings_.get(bindings_.find(key))) {
        existing->flags = 0;
        return;
    }

    bindings_.create({
        .role = role,
        .trigger = *chain
    }, key);

    Logger::info("ActionRouter", "bound '{}' ➜ '{}'", expression, verb);
}

void ActionRouter::tick() {
    for (BindId id = 0; id < bindings_.size(); ++id) {
        Binding* binding = bindings_.get(id);
        if (!binding || binding->trigger.source == InvalidTriggerId)
            continue;

        setFlag(*binding, Down, false);
        setFlag(*binding, Pressed, false);
        setFlag(*binding, Released, false);

        const Lattice::ObjectId object = run_ctx->resolveFocus(Lattice::InvalidFocusScopeId, binding->role);
        if (object == Lattice::InvalidObjectId) continue;

        const TriggerId source = binding->trigger.source;
        const Trigger& trigger = triggers_.require(source);

        auto isDown = [&](TriggerId trigger) {
            for (auto* input : inputs_)
                if (input && input->down(trigger)) return true;
            return false;
        };

        auto matches = [&](const TriggerChain& chain) {
            for (TriggerId required : chain.modifiers)
                if (!isDown(required)) return false;
            return true;
        };

        bool enabled = matches(binding->trigger);

        if (enabled) {
            for (BindId otherId = 0; otherId < bindings_.size(); ++otherId) {
                if (otherId == id) continue;

                const Binding* other = bindings_.get(otherId);
                if (!other || other->trigger.source != binding->trigger.source) continue;
                if (other->trigger.modifiers.size() <= binding->trigger.modifiers.size()) continue;
                if (!matches(other->trigger)) continue;

                if (std::includes(
                    other->trigger.modifiers.begin(), other->trigger.modifiers.end(),
                    binding->trigger.modifiers.begin(), binding->trigger.modifiers.end()
                )) {
                    enabled = false;
                    break;
                }
            }
        }
        
        if (trigger.kind == InputKind::Axis) {
            double value = 0.0;
            if (enabled) for (auto* input : inputs_) if (input)
                value += input->axis(source);
            run_ctx->bindings.set(object, value);
            continue;
        }

        if (trigger.kind == InputKind::Axis2) {
            glm::vec2 value{};
            if (enabled) for (auto* input : inputs_) if (input)
                value += input->axis2(source);
            run_ctx->bindings.set(object, value);
            continue;
        }

        bool now = false;
        if (enabled) {
            for (auto* input : inputs_) {
                if (input && input->down(source)) {
                    now = true;
                    break;
                }
            }
        }

        const bool wasDown = hasFlag(*binding, WasDown);
        const bool pressed = now && !wasDown;
        const bool released = !now && wasDown;

        setFlag(*binding, Down, now);
        setFlag(*binding, Pressed, pressed);
        setFlag(*binding, Released, released);

        const bool fire =
            (binding->mode == ActionMode::OnPress && pressed) ||
            (binding->mode == ActionMode::OnHold && now) ||
            (binding->mode == ActionMode::OnRelease && released);

        if (fire) {
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

        setFlag(*binding, WasDown, now);
    }
}