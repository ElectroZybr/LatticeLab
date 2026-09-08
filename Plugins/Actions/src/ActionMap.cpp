#include "ActionMap.hpp"

#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Settings.hpp>
#include <Lattice/Tools/Logger.hpp>


void ActionMap::configure(Lattice::Node& branch) {
    node_ = &branch;
    // находим все инпуты (устройства ввода)
    inputs_ = branch.globalCollect<InputAPI>();
}

ActionMap::ActionState& ActionMap::ensure(std::string_view verb) {
    return actions_[std::string(verb)];
}

const ActionMap::ActionState* ActionMap::find(std::string_view verb) const {
    auto it = actions_.find(std::string(verb));
    return it == actions_.end() ? nullptr : &it->second;
}

void ActionMap::bind(std::string_view verb, std::string_view trigger, ActionMode mode) {
    ensure(verb);
    bindings_.push_back({std::string(verb), std::string(trigger), mode});
    Logger::ok("ActionMap", "bound '{}' -> '{}'", verb, trigger);
}

void ActionMap::tick() {
    for (auto& [_, s] : actions_)
        s = {};

    if (!node_)
        return;

    auto& kernel = node_->kernel();

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

        auto& s = ensure(b.verb);
        s.down |= now;
        s.pressed |= pressed;
        s.released |= released;

        const bool fire =
            (b.mode == ActionMode::OnPress   && pressed) ||
            (b.mode == ActionMode::OnHold    && now) ||
            (b.mode == ActionMode::OnRelease && released);

        if (fire) {
            Logger::info("ActionMap", "fire: {}", b.verb);
            if (b.target == Target::Action) {
                Lattice::ObjectId id = kernel.objects.resolve(kernel.context, "action", b.verb);
                if (Lattice::ObjectRegistry::valid(id)) kernel.settings.fire(id);
            } else {
                Lattice::ObjectId id = kernel.objects.resolve(kernel.context, "param", b.verb);
                if (!Lattice::ObjectRegistry::valid(id)) continue;
                if (b.target == Target::Toggle)
                    kernel.settings.set(id, !kernel.settings.get<bool>(id));
                else
                    kernel.settings.set(id, kernel.settings.get<double>(id) + b.delta);
            }
        }

        b.wasDown = now;
    }
}

bool ActionMap::down(std::string_view verb) const {
    const auto* s = find(verb);
    return s && s->down;
}

bool ActionMap::pressed(std::string_view verb) const {
    const auto* s = find(verb);
    return s && s->pressed;
}

bool ActionMap::released(std::string_view verb) const {
    const auto* s = find(verb);
    return s && s->released;
}

void ActionMap::clearBinds() {
    bindings_.clear();

    for (auto& [_, state] : actions_)
        state = {};
}