#include <filesystem>
#include <string>
#include <string_view>

#include <glm/vec3.hpp>
#include <Lattice/Composition.hpp>
#include <Actions/InputTargetAPI.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/Render.hpp>
#include <Graphics/Spheres.hpp>
#include <Graphics/TransformMode.hpp>
#include <Graphics/Viewport.hpp>
#include <ParticleDynamics/ParticleSystemAPI.hpp>
#include <Shell/WindowAPI.hpp>
#include <WGPU/GpuSoA.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Graphics",
    .version = {1, 0, 0},
    .dependencies = {{.id = "WGPU", .min = {1, 0, 0}}},
};

namespace {

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

}

// Render branch of one window. The window and its input belong to the shell; here:
//   Render (surface, depth, pipeline layout inside) →
//   GPU data shared by the viewports (a copy of the simulation's particles) →
//   viewports: each is a scene — area, camera, controller and what it draws.
extern "C" void compose(NodeBuild root) {
    auto device = root.mount<WGPU::Device>();
    auto render = root.construct<Render, FrameAPI>(device.ref(), root.require<WindowAPI>("window"));
    render.param("cursor", render->cursor());

    // The GPU copy belongs to the render branch: removing it frees the copy and the
    // simulation keeps running. Copies are commands of the simulation thread.
    auto system = root.mount<ParticleDynamics::ParticleSystemAPI>().ref();
    auto particles = root.emplace<WGPU::GpuSoA>("particles", device.ref(),
        WGPU::GpuSoA::CpuSource{.table = system->particles(), .owner = system->commands()},
        std::string("particles"));
    particles.onRetire([](WGPU::GpuSoA& self) { self.detach(); });
    particles.readyToDestroy([](const WGPU::GpuSoA& self) { return self.idle(); });

    const auto addViewport = [&](std::string_view name, glm::vec3 eye) {
        auto viewport = root.branch(name);
        auto camera = viewport.emplace<Camera>("camera");
        camera->setPosition(eye);
        camera.param("position", camera->transform().position);
        viewport.construct<Viewport>(render->layout(), camera);
        auto controller = viewport.addSlot<TransformController>("controller");
        controller.choice("Graphics::FreeCamera", composeFreeCamera);
        viewport.emplace<Spheres, SceneObject>("particles", render->layout(), particles,
            SphereColumns{
                .x = std::string(Lattice::typeName<Pos::X>()),
                .y = std::string(Lattice::typeName<Pos::Y>()),
                .z = std::string(Lattice::typeName<Pos::Z>()),
            },
            std::filesystem::path("Packages/Graphics/shaders/spheres3d.wgsl"));
    };
    addViewport("main", {0.0f, 0.0f, 5.0f});

    // Input target of this window: the controller of the viewport under the cursor.
    // It stays on the last one when the cursor leaves every viewport.
    render->onHover([active = root.focus<InputTargetAPI>()](Viewport* viewport) mutable {
        if (viewport && viewport->controller)
            active.set(viewport->controller);
    });
}

// Live wiring with the tree: Render draws the current viewports; each viewport draws
// its scene objects and is driven by the controller below it.
extern "C" void configure(NodeConfigure root) {
    root.self<Render>()->setViewports(root.collect<Viewport>());
    for (auto [viewport, node] : root.collectNodes<Viewport>()) {
        viewport->setObjects(node.children<SceneObject>());
        const auto controllers = node.children<TransformController>();
        if (controllers.size() > 1)
            throw Lattice::Exception<Viewport>("Viewport #{} has {} controllers", node.id(), controllers.size());
        viewport->controller = controllers.empty() ? nullptr : controllers.front();
    }
}
