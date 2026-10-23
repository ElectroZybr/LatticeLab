#include <optional>
#include <string>

#include <Lattice/Composition.hpp>
#include <Shell/FrameAPI.hpp>
#include <Shell/KeybindLoader.hpp>
#include <Shell/Window.hpp>
#include <Shell/WindowAPI.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Shell",
    .version = {1, 0, 0},
    .dependencies = {{.id = "Actions", .min = {1, 0, 0}}, {.id = "StdIo", .min = {1, 0, 0}}, {.id = "Graphics", .min = {1, 0, 0}}},
};

// Application shell: keybinds, the render branch (Graphics) and the window loop as
// the host service. The loop collects windows, inputs and frames after the render
// branch is composed.
extern "C" void compose(NodeBuild root) {
    auto router = root.mount<ActionRouter>();
    auto io = root.mount<IOSubsystem>();

    auto keybinds = root.emplace<KeybindsLoader, LoaderAPI>("keybinds", router.ref(), io.ref());
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

    root.compose("Graphics", "render");

    auto loop = root.emplace<WindowSystem>("loop", router.ref(),
        root.collect<WindowAPI>(), root.collect<InputAPI>(), root.collect<FrameAPI>());
    loop.service(&WindowSystem::run, &WindowSystem::requestStop);
}
