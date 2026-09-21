#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>

#include "InputAPI.hpp"


namespace Input {

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,
    X2,
    Count
};

static constexpr std::size_t kMouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

enum class ButtonAction : uint8_t {
    Press,
    Release,
    Repeat
};

struct MouseState {
    bool down[kMouseButtonCount]{};
    bool pressed[kMouseButtonCount]{};
    bool released[kMouseButtonCount]{};

    glm::vec2 pos{0.f};
    glm::vec2 delta{0.f};
    glm::vec2 scroll{0.f};
    glm::vec2 scrollDelta{0.f};

    void beginFrame() {
        std::fill(std::begin(pressed), std::end(pressed), false);
        std::fill(std::begin(released), std::end(released), false);
        delta = {0.f, 0.f};
        scrollDelta = {0.f, 0.f};
    }

    void onButton(MouseButton button, ButtonAction action) {
        const auto i = static_cast<std::size_t>(button);
        if (i >= kMouseButtonCount) return;

        if (action == ButtonAction::Press) {
            if (!down[i]) pressed[i] = true;
            down[i] = true;
        } else if (action == ButtonAction::Release) {
            if (down[i]) released[i] = true;
            down[i] = false;
        }
    }
};

class Mouse final : public InputAPI {
public:
    bool down(std::string_view trigger) const override;
    bool pressed(std::string_view trigger) const override;
    bool released(std::string_view trigger) const override;

    glm::vec2 axis2(std::string_view trigger) const override {
        if (trigger == "MouseDelta") return state_.delta;
        if (trigger == "MouseScroll") return state_.scrollDelta;
        return {};
    }

    void beginFrame() { state_.beginFrame(); }

    void onButton(MouseButton button, ButtonAction action) {
        state_.onButton(button, action);
    }

    void onMove(float x, float y) {
        const glm::vec2 p{x, y};
        state_.delta += p - state_.pos;
        state_.pos = p;
    }

    void onScroll(float dx, float dy) {
        state_.scrollDelta += glm::vec2{dx, dy};
        state_.scroll += glm::vec2{dx, dy};
    }

    void setPosition(float x, float y) {
        state_.pos = {x, y};
    }

    void resetDelta() {
        state_.delta = {0, 0};
    }

    static std::string_view buttonToString(MouseButton button);
    static MouseButton buttonFromString(std::string_view name);

private:
    MouseState state_;
};
}