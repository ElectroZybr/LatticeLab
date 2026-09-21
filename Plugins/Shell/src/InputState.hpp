#pragma once

#include "InputAPI.hpp"

struct ButtonState {
    bool down = false;
    bool pressed = false;
    bool released = false;
};

class InputState {
public:
    void ensure(TriggerId id) {
        if (id == InvalidTriggerId) return;
        const size_t size = size_t(id) + 1;
        if (buttons_.size() < size) buttons_.resize(size);
        if (axes_.size() < size) axes_.resize(size);
        if (axes2_.size() < size) axes2_.resize(size);
    }

    void beginFrame() {
        for (auto& state : buttons_) {
            state.pressed = false;
            state.released = false;
        }
        std::fill(axes_.begin(), axes_.end(), 0.0);
        std::fill(axes2_.begin(), axes2_.end(), glm::vec2{});
    }

    void button(TriggerId id, bool down) {
        if (id == InvalidTriggerId) return;
        ensure(id);
        auto& state = buttons_[id];
        if (down && !state.down) state.pressed = true;
        if (!down && state.down) state.released = true;
        state.down = down;
    }

    void setAxis(TriggerId id, double value) { if (id == InvalidTriggerId) return; ensure(id); axes_[id] = value; }
    void addAxis(TriggerId id, double value) { if (id == InvalidTriggerId) return; ensure(id); axes_[id] += value; }

    void setAxis2(TriggerId id, glm::vec2 value) { if (id == InvalidTriggerId) return; ensure(id); axes2_[id] = value; }
    void addAxis2(TriggerId id, glm::vec2 value) { if (id == InvalidTriggerId) return; ensure(id); axes2_[id] += value; }

    bool down(TriggerId id) const { return id < buttons_.size() && buttons_[id].down; }
    bool pressed(TriggerId id) const { return id < buttons_.size() && buttons_[id].pressed; }
    bool released(TriggerId id) const { return id < buttons_.size() && buttons_[id].released; }
    double axis(TriggerId id) const { return id < axes_.size() ? axes_[id] : 0.0; }
    glm::vec2 axis2(TriggerId id) const { return id < axes2_.size() ? axes2_[id] : glm::vec2{}; }

private:
    std::vector<ButtonState> buttons_;
    std::vector<double> axes_;
    std::vector<glm::vec2> axes2_;
};