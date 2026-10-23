// Keybinds loaded through IO into the router; no window or GPU required.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <Lattice/Kernel/Context.hpp>
#include <StdIo/TomlParser.hpp>

#include <Shell/KeybindLoader.hpp>

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
    }
    runtime.nodes.ops.destroyBranch(rootId);
    std::filesystem::remove(file);
    std::cout << "Shell: keybinds load through IO and drive the router\n";
}
