#include <optional>
#include <string>

#include <Lattice/Composition.hpp>
#include <actions/InputTargetAPI.hpp>
#include <shell/FrameAPI.hpp>
#include <shell/GlfwKeyboard.hpp>
#include <shell/GlfwMouse.hpp>
#include <shell/GlfwWindow.hpp>
#include <shell/KeybindLoader.hpp>
#include <shell/Window.hpp>
#include <shell/WindowAPI.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Shell",
    .version = {1, 0, 0},
    .dependencies = {{.id = "StdIo", .min = {1, 0, 0}}, {.id = "Graphics", .min = {1, 0, 0}}},
};

namespace {

// Window variant: the window owns its input devices.
auto composeGlfwWindow(NodeBuild& slot) {
    auto keyboard = slot.emplace<GlfwKeyboard, InputAPI>("keyboard");
    auto mouse = slot.emplace<GlfwMouse, InputAPI>("mouse");
    return slot.construct<GlfwWindow, WindowAPI>(keyboard, mouse);
}

}

// Application shell: the input router, keybinds, the window with its input, the
// render branch of the window (Graphics) and the window loop as the host service.
extern "C" void compose(NodeBuild root) {
    auto io = root.mount<IOSubsystem>();
    auto router = root.emplace<ActionRouter>("router", root.exports(), root.id());

    auto keybinds = root.emplace<KeybindsLoader, LoaderAPI>("keybinds", router, io.ref());
    const auto load = keybinds.action("load", &KeybindsLoader::loadFile);
    // Startup calls load on the composition root; forward through Exports so a
    // retired loader rejects the call instead of using a stale pointer.
    root.action<std::optional<std::string>>("load",
        [exports = &keybinds.runtime()->exports, load](Lattice::ActionContext& context, std::optional<std::string> path) {
            if (path) {
                const Lattice::Value argument{*path};
                exports->invoke(load, context, {&argument, 1});
            } else exports->invoke(load, context);
        });

    auto window = root.addSlot<WindowAPI>("window");
    window.choice("Shell::Glfw", composeGlfwWindow);
    root.compose("Graphics", "render");

    auto loop = root.construct<WindowSystem>(router);
    loop.service(&WindowSystem::run, &WindowSystem::requestStop);
}

// The loop serves the windows, inputs and frames currently in the shell branch
// (the render branch included); it picks changes up on its next idle().
extern "C" void configure(NodeConfigure root) {
    // Keybinds resolve from the input target chosen in the frame's branch, or from the
    // frame node itself (its local exports, e.g. the cursor) while none is chosen.
    const auto frames = root.collectNodes<FrameAPI>();
    if (frames.size() > 1)
        throw Lattice::Exception<WindowSystem>("{} frames share one router; input focus between windows is not supported yet",
            frames.size());
    Lattice::NodeId target = Lattice::InvalidNodeId;
    if (!frames.empty()) {
        target = frames.front().node.focusId<InputTargetAPI>();
        if (target == Lattice::InvalidNodeId)
            target = frames.front().node.id();
    }
    root.require<ActionRouter>("router").setTarget(target);

    auto* loop = root.self<WindowSystem>();
    loop->setWindows(root.collect<WindowAPI>());
    loop->setInputs(root.collect<InputAPI>());
    loop->setFrames(root.collect<FrameAPI>());
}
