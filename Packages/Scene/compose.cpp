#include <filesystem>
#include <string_view>

#include <glm/vec3.hpp>
#include <Lattice/Composition.hpp>
#include <Actions/InputTargetAPI.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/Render.hpp>
#include <Graphics/RenderLayout.hpp>
#include <Graphics/TestObject.hpp>
#include <Graphics/TransformMode.hpp>
#include <Graphics/Viewport.hpp>
#include <Shell/GlfwKeyboard.hpp>
#include <Shell/GlfwMouse.hpp>
#include <Shell/GlfwWindow.hpp>
#include <WGPU/Resources.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Scene",
    .version = {1, 0, 0},
    .dependencies = {{.id = "WGPU", .min = {1, 0, 0}}},
};

namespace {

// Window variant: the window owns its input devices.
auto composeGlfwWindow(NodeBuild& slot) {
    auto keyboard = slot.emplace<GlfwKeyboard, InputAPI>("keyboard");
    auto mouse = slot.emplace<GlfwMouse, InputAPI>("mouse");
    return slot.construct<GlfwWindow, WindowAPI>(keyboard, mouse);
}

// Input of any camera controller, declared once on TransformController.
template<class T>
Lattice::ObjectBuild<T> exportControls(Lattice::ObjectBuild<T> controller) {
    controller.param("look", &TransformController::look);
    controller.param("zoom", &TransformController::zoom);
    controller.param("pan", &TransformController::pan);
    controller.param("move", &TransformController::move);
    controller.action("forward", [](TransformController& self) { self.move.z += 1; });
    controller.action("backward", [](TransformController& self) { self.move.z -= 1; });
    controller.action("left", [](TransformController& self) { self.move.x -= 1; });
    controller.action("right", [](TransformController& self) { self.move.x += 1; });
    controller.action("up", [](TransformController& self) { self.move.y += 1; });
    controller.action("down", [](TransformController& self) { self.move.y -= 1; });
    return controller;
}

// Controller variants live on the slot node itself; the controller belongs to the
// viewport that owns the slot.
auto composeFreeCamera(NodeBuild& slot) {
    return exportControls(slot.construct<FreeCameraController, TransformController, InputTargetAPI>(
        slot.require<Camera>("camera"), slot.owner<Viewport>()));
}

auto composeOrbitCamera(NodeBuild& slot) {
    return exportControls(slot.construct<OrbitCameraController, TransformController, InputTargetAPI>(
        slot.require<Camera>("camera"), slot.owner<Viewport>()));
}

// A viewport is its own node: the viewport object, its camera and its controller.
void addViewport(NodeBuild& root, const Ref<RenderLayout>& layout, std::string_view name, glm::vec3 eye) {
    auto viewport = root.branch(name);
    auto camera = viewport.emplace<Camera>("camera");
    camera->setPosition(eye);
    camera.param("position", camera->transform().position);
    viewport.construct<Viewport>(layout, camera);
    auto controller = viewport.addSlot<TransformController>("controller");
    controller.choice("Scene::FreeCamera", composeFreeCamera);
}

}

// Scene of one window, assembled in code:
//   window and surface → the window's RenderLayout →
//   scene objects (direct children: every viewport draws them) →
//   viewports (camera, controller) → Render (draws the viewports, owns cursor and depth).
extern "C" void compose(NodeBuild root) {
    auto device = root.mount<WGPU::Device>();

    auto window = root.addSlot<WindowAPI>("window");
    window.choice("Shell::Glfw", composeGlfwWindow);
    auto surface = root.emplace<WGPU::Surface>("surface", device.ref(), window.get()->native());
    auto layout = root.emplace<RenderLayout>("layout", device.ref(), surface->format());

    auto marker = root.emplace<TestObject, SceneObject>("marker", layout,
        std::filesystem::path("Packages/Graphics/shaders/circle.wgsl"));
    marker.param("position", marker->transform().position);

    addViewport(root, layout.ref(), "main", {0.0f, 0.0f, 2.0f});

    auto render = root.construct<Render, FrameAPI>(window.ref(), surface, layout);
    render.param("cursor", render->cursor());

    // Input target of this window: the controller of the viewport under the cursor.
    // It stays on the last one when the cursor leaves every viewport.
    render->onHover([active = root.focus<InputTargetAPI>()](Viewport* viewport) mutable {
        if (viewport && viewport->controller)
            active.set(viewport->controller);
    });
}

// Live wiring with the tree: Render draws the current viewports; each viewport draws
// the scene objects of the branch plus its own and is driven by the controller below it.
extern "C" void configure(NodeConfigure root) {
    root.self<Render>()->setViewports(root.collect<Viewport>());
    const auto scene = root.children<SceneObject>();
    for (auto [viewport, node] : root.collectNodes<Viewport>()) {
        auto objects = scene;
        for (SceneObject* own : node.children<SceneObject>())
            objects.push_back(own);
        viewport->setObjects(std::move(objects));

        const auto controllers = node.children<TransformController>();
        if (controllers.size() > 1)
            throw Lattice::Exception<Viewport>("Viewport #{} has {} controllers", node.id(), controllers.size());
        viewport->controller = controllers.empty() ? nullptr : controllers.front();
    }
}
