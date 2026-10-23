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
    auto controller = slot.emplace<FreeCameraController>("free",
        slot.require<Camera>("camera"), slot.require<Viewport>("view"));
    controller.param("look", &FreeCameraController::look);
    controller.param("zoom", &FreeCameraController::zoom);
    controller.param("pan", &FreeCameraController::pan);
    controller.param("move", &FreeCameraController::move);
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

    auto viewport = root.branch("viewport");
    auto camera = viewport.emplace<Camera>("camera");
    camera->setPosition({0.0f, 0.0f, 2.0f});
    auto view = viewport.emplace<TestViewport, Viewport>("view",
        device.ref(), surface->format(), camera, std::string{"Packages/Graphics/shaders/circle.wgsl"});
    view.param("cursor", view->cursorValue());
    auto controller = viewport.addSlot<TransformController>("controller");
    controller.choice("Graphics::FreeCamera", composeFreeCamera);

    root.emplace<Render, FrameAPI>("render", device.ref(), window.ref(), surface,
        root.collect<Viewport>(), root.collect<TransformController>());
}
