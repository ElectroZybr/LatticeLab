// Router over runtime exports; inputs are plain test objects.
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <Lattice/Kernel/Context.hpp>
#include <Actions/ActionRouter.hpp>

using namespace Lattice;

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TestInput final : public InputAPI {
public:
    std::set<std::string> held;
    void registerTriggers(TriggerRegistry& triggers) override {
        for (const char* name : {"A", "B", "Shift"})
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

struct Settings {
    bool enabled = false;
    double speed = 1.0;
    int fired = 0;
};
}

int main() {
    Settings settings;
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        root.globalParam("enabled", settings.enabled);
        root.globalParam("speed", settings.speed);
        root.globalAction("fire", [&] { ++settings.fired; });

        TestInput input;
        ActionRouter router{root.exports(), rootId, {&input}};
        router.bind("fire", "A");
        router.bindToggle("enabled", "A+Shift");
        router.bindAdd("speed", "B", 0.5);

        input.held = {"A"};
        router.tick();
        router.tick();
        check(settings.fired == 1, "OnPress fires once while held");

        input.held = {};
        router.tick();
        input.held = {"A", "Shift"};
        router.tick();
        check(settings.enabled && settings.fired == 1, "more specific chain wins");

        input.held = {"B"};
        router.tick();
        check(settings.speed == 1.5, "add binding changes param");
        check(router.down(runtime.nodes.context.role("speed")), "binding state is queryable");

        router.unregisterInput(input);
        input.held = {"A"};
        router.tick();
        check(settings.fired == 1, "unregistered input is ignored");
    }
    runtime.nodes.ops.destroyBranch(rootId);
    std::cout << "Actions: press, modifier chains and param bindings passed\n";
}
