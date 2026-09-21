#pragma once

#include "InputState.hpp"
#include <GLFW/glfw3.h>
#include "Lattice/Kernel/Node.hpp"

class glfwKeyboard final : public InputAPI {
public:
    explicit glfwKeyboard(Lattice::Node&) {
        keys_.fill(InvalidTriggerId);
    }

    void registerTriggers(TriggerRegistry& triggers) override {
        keys_.fill(InvalidTriggerId);
        
        for (const auto& desc : kGlfwKeys) {
            TriggerId id = triggers.find(desc.name);

            if (!TriggerRegistry::valid(id))
                id = triggers.create({.name = std::string(desc.name), .kind = InputKind::Button});

            keys_[desc.glfw] = id;
            state_.ensure(id);

            for (auto alias : desc.aliases)
                triggers.alias(id, std::string(alias));
        }

        for (size_t i = 0; i < std::size(kVirtualKeys); ++i) {
            const auto& desc = kVirtualKeys[i];

            TriggerId id = triggers.find(desc.name);
            if (!TriggerRegistry::valid(id))
                id = triggers.create({.name = std::string(desc.name), .kind = InputKind::Button});

            virtualKeys_[i] = {
                .id = id,
                .left = desc.left,
                .right = desc.right
            };

            state_.ensure(id);
        }
    }

    bool down(TriggerId id) const override { return state_.down(id); }
    bool pressed(TriggerId id) const override { return state_.pressed(id); }
    bool released(TriggerId id) const override { return state_.released(id); }

    double axis(TriggerId id) const override { return state_.axis(id); }
    glm::vec2 axis2(TriggerId id) const override { return state_.axis2(id); }

    void beginFrame() { state_.beginFrame(); }

    void onKey(int key, int action) {
        if (key < 0 || key >= int(keys_.size())) return;

        const TriggerId id = keys_[key];
        if (id == InvalidTriggerId) return;

        if (action == GLFW_PRESS)
            state_.button(id, true);
        else if (action == GLFW_RELEASE)
            state_.button(id, false);
        else
            return;

        for (const auto& virtualKey : virtualKeys_) {
            if (key != virtualKey.left && key != virtualKey.right)
                continue;

            state_.button(
                virtualKey.id,
                state_.down(keys_[virtualKey.left]) ||
                state_.down(keys_[virtualKey.right])
            );

            break;
        }
    }

private:
    InputState state_;

    struct VirtualKey {
        TriggerId id;
        int left;
        int right;
    };

    struct VirtualKeyDesc {
        std::string_view name;
        int left;
        int right;
    };

    static constexpr VirtualKeyDesc kVirtualKeys[] = {
        {"Ctrl",  GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL},
        {"Shift", GLFW_KEY_LEFT_SHIFT,   GLFW_KEY_RIGHT_SHIFT},
        {"Alt",   GLFW_KEY_LEFT_ALT,     GLFW_KEY_RIGHT_ALT},
        {"Super", GLFW_KEY_LEFT_SUPER,   GLFW_KEY_RIGHT_SUPER}
    };

    std::array<VirtualKey, std::size(kVirtualKeys)> virtualKeys_{};

    struct KeyDesc {
        int glfw;
        std::string_view name;
        std::initializer_list<std::string_view> aliases;
    };

    std::array<TriggerId, GLFW_KEY_LAST + 1> keys_;

    static constexpr KeyDesc kGlfwKeys[] = {
        {GLFW_KEY_A, "A", {}},
        {GLFW_KEY_B, "B", {}},
        {GLFW_KEY_C, "C", {}},
        {GLFW_KEY_D, "D", {}},
        {GLFW_KEY_E, "E", {}},
        {GLFW_KEY_F, "F", {}},
        {GLFW_KEY_G, "G", {}},
        {GLFW_KEY_H, "H", {}},
        {GLFW_KEY_I, "I", {}},
        {GLFW_KEY_J, "J", {}},
        {GLFW_KEY_K, "K", {}},
        {GLFW_KEY_L, "L", {}},
        {GLFW_KEY_M, "M", {}},
        {GLFW_KEY_N, "N", {}},
        {GLFW_KEY_O, "O", {}},
        {GLFW_KEY_P, "P", {}},
        {GLFW_KEY_Q, "Q", {}},
        {GLFW_KEY_R, "R", {}},
        {GLFW_KEY_S, "S", {}},
        {GLFW_KEY_T, "T", {}},
        {GLFW_KEY_U, "U", {}},
        {GLFW_KEY_V, "V", {}},
        {GLFW_KEY_W, "W", {}},
        {GLFW_KEY_X, "X", {}},
        {GLFW_KEY_Y, "Y", {}},
        {GLFW_KEY_Z, "Z", {}},

        {GLFW_KEY_0, "0", {}},
        {GLFW_KEY_1, "1", {}},
        {GLFW_KEY_2, "2", {}},
        {GLFW_KEY_3, "3", {}},
        {GLFW_KEY_4, "4", {}},
        {GLFW_KEY_5, "5", {}},
        {GLFW_KEY_6, "6", {}},
        {GLFW_KEY_7, "7", {}},
        {GLFW_KEY_8, "8", {}},
        {GLFW_KEY_9, "9", {}},

        {GLFW_KEY_SPACE, "Space", {" "}},
        {GLFW_KEY_APOSTROPHE, "Apostrophe", {"'"}},
        {GLFW_KEY_COMMA, "Comma", {","}},
        {GLFW_KEY_MINUS, "Minus", {"-"}},
        {GLFW_KEY_PERIOD, "Period", {"."}},
        {GLFW_KEY_SLASH, "Slash", {"/"}},
        {GLFW_KEY_SEMICOLON, "Semicolon", {";"}},
        {GLFW_KEY_EQUAL, "Equal", {"="}},
        {GLFW_KEY_LEFT_BRACKET, "LeftBracket", {"["}},
        {GLFW_KEY_BACKSLASH, "Backslash", {"\\"}},
        {GLFW_KEY_RIGHT_BRACKET, "RightBracket", {"]"}},
        {GLFW_KEY_GRAVE_ACCENT, "GraveAccent", {"`"}},

        {GLFW_KEY_WORLD_1, "World1", {}},
        {GLFW_KEY_WORLD_2, "World2", {}},

        {GLFW_KEY_ESCAPE, "Escape", {"Esc"}},
        {GLFW_KEY_ENTER, "Enter", {"Return"}},
        {GLFW_KEY_TAB, "Tab", {}},
        {GLFW_KEY_BACKSPACE, "Backspace", {}},
        {GLFW_KEY_INSERT, "Insert", {"Ins"}},
        {GLFW_KEY_DELETE, "Delete", {"Del"}},

        {GLFW_KEY_RIGHT, "Right", {}},
        {GLFW_KEY_LEFT, "Left", {}},
        {GLFW_KEY_DOWN, "Down", {}},
        {GLFW_KEY_UP, "Up", {}},

        {GLFW_KEY_PAGE_UP, "PageUp", {"PgUp"}},
        {GLFW_KEY_PAGE_DOWN, "PageDown", {"PgDown"}},
        {GLFW_KEY_HOME, "Home", {}},
        {GLFW_KEY_END, "End", {}},

        {GLFW_KEY_CAPS_LOCK, "CapsLock", {}},
        {GLFW_KEY_SCROLL_LOCK, "ScrollLock", {}},
        {GLFW_KEY_NUM_LOCK, "NumLock", {}},
        {GLFW_KEY_PRINT_SCREEN, "PrintScreen", {"PrintScr"}},
        {GLFW_KEY_PAUSE, "Pause", {}},

        {GLFW_KEY_F1, "F1", {}},
        {GLFW_KEY_F2, "F2", {}},
        {GLFW_KEY_F3, "F3", {}},
        {GLFW_KEY_F4, "F4", {}},
        {GLFW_KEY_F5, "F5", {}},
        {GLFW_KEY_F6, "F6", {}},
        {GLFW_KEY_F7, "F7", {}},
        {GLFW_KEY_F8, "F8", {}},
        {GLFW_KEY_F9, "F9", {}},
        {GLFW_KEY_F10, "F10", {}},
        {GLFW_KEY_F11, "F11", {}},
        {GLFW_KEY_F12, "F12", {}},
        {GLFW_KEY_F13, "F13", {}},
        {GLFW_KEY_F14, "F14", {}},
        {GLFW_KEY_F15, "F15", {}},
        {GLFW_KEY_F16, "F16", {}},
        {GLFW_KEY_F17, "F17", {}},
        {GLFW_KEY_F18, "F18", {}},
        {GLFW_KEY_F19, "F19", {}},
        {GLFW_KEY_F20, "F20", {}},
        {GLFW_KEY_F21, "F21", {}},
        {GLFW_KEY_F22, "F22", {}},
        {GLFW_KEY_F23, "F23", {}},
        {GLFW_KEY_F24, "F24", {}},
        {GLFW_KEY_F25, "F25", {}},

        {GLFW_KEY_KP_0, "Kp0", {}},
        {GLFW_KEY_KP_1, "Kp1", {}},
        {GLFW_KEY_KP_2, "Kp2", {}},
        {GLFW_KEY_KP_3, "Kp3", {}},
        {GLFW_KEY_KP_4, "Kp4", {}},
        {GLFW_KEY_KP_5, "Kp5", {}},
        {GLFW_KEY_KP_6, "Kp6", {}},
        {GLFW_KEY_KP_7, "Kp7", {}},
        {GLFW_KEY_KP_8, "Kp8", {}},
        {GLFW_KEY_KP_9, "Kp9", {}},

        {GLFW_KEY_KP_DECIMAL, "KpDecimal", {}},
        {GLFW_KEY_KP_DIVIDE, "KpDivide", {}},
        {GLFW_KEY_KP_MULTIPLY, "KpMultiply", {}},
        {GLFW_KEY_KP_SUBTRACT, "KpSubtract", {}},
        {GLFW_KEY_KP_ADD, "KpAdd", {}},
        {GLFW_KEY_KP_ENTER, "KpEnter", {}},
        {GLFW_KEY_KP_EQUAL, "KpEqual", {}},

        {GLFW_KEY_LEFT_SHIFT, "LeftShift", {"LShift"}},
        {GLFW_KEY_LEFT_CONTROL, "LeftCtrl", {"LCtrl"}},
        {GLFW_KEY_LEFT_ALT, "LeftAlt", {"LAlt"}},
        {GLFW_KEY_LEFT_SUPER, "LeftSuper", {"LSuper"}},

        {GLFW_KEY_RIGHT_SHIFT, "RightShift", {"RShift"}},
        {GLFW_KEY_RIGHT_CONTROL, "RightCtrl", {"RCtrl"}},
        {GLFW_KEY_RIGHT_ALT, "RightAlt", {"RAlt"}},
        {GLFW_KEY_RIGHT_SUPER, "RightSuper", {"RSuper"}},

        {GLFW_KEY_MENU, "Menu", {}}
    };

};