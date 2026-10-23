#include <string>

#include <Lattice/Composition.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/Render.hpp>
#include <Graphics/TestViewport.hpp>
#include <Graphics/TransformMode.hpp>
#include <Shell/glfwKeyboard.hpp>
#include <Shell/glfwMouse.hpp>
#include <Shell/glfwWindow.hpp>
#include <WGPU/Resources.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Graphics",
    .version = {1, 0, 0},
    .dependencies = {{.id = "WGPU", .min = {1, 0, 0}}},
};

namespace {

// Window variant: the window owns its input devices.
auto composeGlfwWindow(NodeBuild& slot) {
    auto keyboard = slot.emplace<glfwKeyboard, InputAPI>("keyboard");
    auto mouse = slot.emplace<glfwMouse, InputAPI>("mouse");
    return slot.emplace<glfwWindow>("glfw", keyboard, mouse);
}

auto composeFreeCamera(NodeBuild& slot) {
    // The controller belongs to the viewport that owns its slot.
    auto controller = slot.emplace<FreeCameraController>("free",
        slot.require<Camera>("camera"), slot.owner<Viewport>());
    auto ref = controller.ref();
    NodeBuild controls{ref.id(), *ref.runtime()};
    controls.globalParam("look", controller->look);
    controls.globalParam("zoom", controller->zoom);
    controls.globalParam("pan", controller->pan);
    controls.globalParam("move", controller->move);
    controls.globalAction("forward", [ref] { ref->move.z += 1; });
    controls.globalAction("backward", [ref] { ref->move.z -= 1; });
    controls.globalAction("left", [ref] { ref->move.x -= 1; });
    controls.globalAction("right", [ref] { ref->move.x += 1; });
    controls.globalAction("up", [ref] { ref->move.y += 1; });
    controls.globalAction("down", [ref] { ref->move.y -= 1; });
    return controller;
}

}

// Render branch of one window: the window, its surface, viewports and the renderer.
// Mounted resources come from the surrounding environment (WGPU device).
extern "C" void compose(NodeBuild root) {
    auto device = root.mount<WGPU::Device>();

    auto window = root.addSlot<WindowAPI>("window");
    window.choice("Shell::Glfw", composeGlfwWindow);

    auto surface = root.emplace<WGPU::Surface>("surface", device.ref(), window.get()->native());

    // Each viewport is its own node: the viewport object with its camera and controller.
    // Render draws every viewport of the branch, so more can be added the same way.
    const auto addViewport = [&](std::string_view name, std::string shader) {
        auto viewport = root.branch(name);
        auto camera = viewport.emplace<Camera>("camera");
        camera->setPosition({0.0f, 0.0f, 2.0f});
        auto view = viewport.construct<TestViewport, Viewport>(
            device.ref(), surface->format(), camera, std::move(shader));
        viewport.globalParam("cursor", view->cursorValue());
        auto controller = viewport.addSlot<TransformController>("controller");
        controller.choice("Graphics::FreeCamera", composeFreeCamera);
    };
    addViewport("viewport", "Packages/Graphics/shaders/circle.wgsl");

    root.emplace<Render, FrameAPI>("render", device.ref(), window.ref(), surface,
        root.collect<Viewport>(), root.collect<TransformController>());
}
