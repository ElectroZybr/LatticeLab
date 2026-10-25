// Keybinds loaded through IO into the router; no window or GPU required.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <Lattice/kernel/Context.hpp>
#include <stdio/TomlParser.hpp>

#include <shell/KeybindLoader.hpp>
#include <shell/GlfwKeyboard.hpp>
#include <shell/GlfwMouse.hpp>

using namespace Lattice;

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TestInput final : public InputAPI {
public:
    std::set<std::string> held;
    void registerTriggers(TriggerRegistry& triggers) override {
        for (const char* name : {"A", "B"})
            ids_[name] = triggers.create({.name = name}, name);
    }
    bool down(TriggerId trigger) const override {
        for (const auto& name : held)
            if (ids_.at(name) == trigger) return true;
        return false;
    }
    bool pressed(TriggerId) const override { return false; }
    bool released(TriggerId) const override { return false; }
private:
    std::unordered_map<std::string, TriggerId> ids_;
};
}

int main() {
    const auto file = std::filesystem::temp_directory_path() /
        ("shell-keybinds-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".toml");
    std::ofstream(file) << "[keybinds]\nfire = \"A\"\nspeed.add = [\"B\", 0.5, \"hold\"]\n";

    int fired = 0;
    double speed = 1.0;
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        root.globalAction("fire", [&] { ++fired; });
        root.globalParam("speed", speed);

        TestInput input;
        TomlParser toml;
        IOSubsystem io{{&toml}};
        ActionRouter router{root.exports(), rootId, {&input}};
        KeybindsLoader keybinds{router, io};
        keybinds.loadFile(file.string());

        input.held = {"A", "B"};
        router.tick();
        router.tick();
        check(fired == 1, "press binding fires once");
        check(speed == 2.0, "hold binding adds every frame");
        router.unregisterInput(input);

        GlfwKeyboard keyboard;
        GlfwMouse mouse;
        router.registerInput(keyboard);
        router.registerInput(mouse);
        glm::vec2 look{}, cursor{};
        double zoom = 0;
        int forward = 0;
        root.globalAction("forward", [&] { ++forward; });
        root.globalParam("look", look);
        root.globalParam("zoom", zoom);
        root.globalParam("cursor", cursor);
        keybinds.load(Value{Object{
            {"forward", Array{"W", "hold"}},
            {"look", Array{"MouseDelta+MouseRight", "axis2"}},
            {"zoom", Array{"MouseWheel", "axis"}},
            {"cursor", Array{"MousePos", "axis2"}}
        }});
        keyboard.onKey(GLFW_KEY_W, GLFW_PRESS);
        mouse.onMove(100, 100);
        mouse.onMove(104, 106);
        mouse.onButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
        mouse.onScroll(0, 2);
        router.tick();
        check(forward == 1 && look == glm::vec2(4, 6) && zoom == 2, "keyboard/mouse bindings reach runtime exports");
        mouse.beginFrame();
        mouse.onButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
        router.tick();
        check(forward == 2 && look == glm::vec2{} && zoom == 0, "hold repeats while mouse deltas clear");
        check(cursor == glm::vec2(104, 106), "absolute cursor survives an idle frame");
        router.unregisterInput(mouse);
        router.unregisterInput(keyboard);
    }
    runtime.nodes.ops.destroyBranch(rootId);
    std::filesystem::remove(file);
    std::cout << "Shell: keybinds load through IO and drive the router\n";
}
