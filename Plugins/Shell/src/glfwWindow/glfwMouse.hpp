#pragma once

#include "InputState.hpp"
#include <GLFW/glfw3.h>
#include "Lattice/Kernel/Node.hpp"

class glfwMouse final : public InputAPI {
public:
    explicit glfwMouse(Lattice::Node&) {}
    void registerTriggers(TriggerRegistry& triggers) override {
        buttons_.fill(InvalidTriggerId);
        auto add = [&](std::string_view name, InputKind kind) {
            if (auto id = triggers.find(name); TriggerRegistry::valid(id)) return id;
            return triggers.create({.name = std::string(name), .kind = kind});
        };

        buttons_[GLFW_MOUSE_BUTTON_LEFT]   = add("MouseLeft", InputKind::Button);
        buttons_[GLFW_MOUSE_BUTTON_RIGHT]  = add("MouseRight", InputKind::Button);
        buttons_[GLFW_MOUSE_BUTTON_MIDDLE] = add("MouseMiddle", InputKind::Button);
        buttons_[GLFW_MOUSE_BUTTON_4]      = add("MouseX1", InputKind::Button);
        buttons_[GLFW_MOUSE_BUTTON_5]      = add("MouseX2", InputKind::Button);

        position_ = add("MousePos", InputKind::Axis2);
        delta_    = add("MouseDelta", InputKind::Axis2);
        wheel_    = add("MouseWheel", InputKind::Axis);

        state_.ensure(position_);
        state_.ensure(delta_);
        state_.ensure(wheel_);
    }

    bool down(TriggerId id) const override { return state_.down(id); }
    bool pressed(TriggerId id) const override { return state_.pressed(id); }
    bool released(TriggerId id) const override { return state_.released(id); }
    double axis(TriggerId id) const override { return state_.axis(id); }
    glm::vec2 axis2(TriggerId id) const override { return state_.axis2(id); }

    void beginFrame() { state_.beginFrame(); }

    void onMove(double x, double y) {
        const glm::vec2 position{float(x), float(y)};

        if (positionInitialized_)
            state_.addAxis2(delta_, position - positionValue_);
        else
            positionInitialized_ = true;

        positionValue_ = position;
        state_.setAxis2(position_, position);
    }

    void onButton(int button, int action) {
        if (button < 0 || button >= int(buttons_.size())) return;
        const TriggerId id = buttons_[button];
        if (id == InvalidTriggerId) return;
        state_.button(id, action != GLFW_RELEASE);
    }

    void onScroll(double, double y) {
        state_.addAxis(wheel_, y);
    }

private:
    InputState state_;

    std::array<TriggerId, GLFW_MOUSE_BUTTON_LAST + 1> buttons_{};

    TriggerId position_ = InvalidTriggerId;
    TriggerId delta_ = InvalidTriggerId;
    TriggerId wheel_ = InvalidTriggerId;

    glm::vec2 positionValue_{};
    bool positionInitialized_ = false;
};