#include <filesystem>
#include <string>
#include <string_view>

#include <glm/vec3.hpp>
#include <Lattice/Composition.hpp>
#include <actions/InputTargetAPI.hpp>
#include <graphics/Camera.hpp>
#include <graphics/GpuBoundaries.hpp>
#include <graphics/GpuMesh.hpp>
#include <graphics/Light.hpp>
#include <graphics/MeshObject.hpp>
#include <graphics/MeshShapes.hpp>
#include <graphics/SdfMesh.hpp>
#include <graphics/Render.hpp>
#include <graphics/Spheres.hpp>
#include <graphics/TransformMode.hpp>
#include <graphics/Viewport.hpp>
#include <particledynamics/ParticleSystemAPI.hpp>
#include <shell/WindowAPI.hpp>
#include <wgpu/GpuSoA.hpp>

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

// Material and display modes of a drawn mesh.
void exportMesh(Lattice::ObjectBuild<MeshObject>& mesh) {
    mesh.param("display", &MeshObject::display);
    mesh.param("color", &MeshObject::color);
    mesh.param("opacity", &MeshObject::opacity);
    mesh.param("specular", &MeshObject::specular);
    mesh.param("shininess", &MeshObject::shininess);
    mesh.param("cullBack", &MeshObject::cullBack);
    mesh.param("fillColor", &MeshObject::fillColor);
    mesh.param("edgeColor", &MeshObject::edgeColor);
    mesh.param("edgeWidth", &MeshObject::edgeWidth);
    mesh.param("hiddenColor", &MeshObject::hiddenColor);
    mesh.param("hiddenWidth", &MeshObject::hiddenWidth);
    mesh.param("dashLength", &MeshObject::dashLength);
    mesh.param("creaseAngle", &MeshObject::creaseAngle);
    mesh.param("triangles", &MeshObject::triangles);
    mesh.param("triangleColor", &MeshObject::triangleColor);
    mesh.param("triangleWidth", &MeshObject::triangleWidth);
}

Lattice::ObjectBuild<Light> addLight(NodeBuild& scene, std::string_view name, Light light) {
    auto node = scene.emplace<Light>(name, light);
    node.param("color", &Light::color);
    node.param("intensity", &Light::intensity);
    node.param("point", &Light::point);
    node.param("direction", &Light::direction);
    node.param("position", &Light::position);
    node.param("range", &Light::range);
    return node;
}


}

// Render branch of one window. The window and its input belong to the shell; here:
//   Render (surface, depth, pipeline layout inside) →
//   GPU data shared by the viewports (a copy of the simulation's particles) →
//   viewports: each is a scene — area, camera, controller and what it draws.
extern "C" void compose(NodeBuild root) {
    auto device = root.mount<Wgpu::Device>();
    auto render = root.construct<Render, FrameAPI>(device.ref(), root.require<WindowAPI>("window"));
    render.param("cursor", render->cursor());

    // The GPU copy belongs to the render branch: removing it frees the copy and the
    // simulation keeps running. Copies are commands of the simulation thread.
    auto system = root.mount<ParticleDynamics::ParticleSystemAPI>().ref();
    auto particles = root.emplace<Wgpu::GpuSoA>("particles", device.ref(),
        Wgpu::GpuSoA::CpuSource{.table = system->particles(), .owner = system->commands()},
        std::string("particles"));
    particles.onRetire([](Wgpu::GpuSoA& self) { self.detach(); });
    particles.readyToDestroy([](const Wgpu::GpuSoA& self) { return self.idle(); });

    // The simulation's boundaries, copied like the particles; their surface as a mesh.
    auto boundaries = root.emplace<GpuBoundaries>("boundaries", device.ref(), system);
    boundaries.onMaintain([](GpuBoundaries& self) { self.update(); });
    boundaries.onRetire([](GpuBoundaries& self) { self.detach(); });
    boundaries.readyToDestroy([](const GpuBoundaries& self) { return self.idle(); });
    auto walls = root.emplace<SdfMesh>("walls", device.ref(), boundaries, 128u);
    walls.param("resolution", &SdfMesh::resolution);
    walls.onMaintain([](SdfMesh& mesh) { mesh.update(); });

    // Demo mesh: shared data, drawn by the viewports' mesh objects.
    auto torus = root.emplace<GpuMesh>("torus", device.ref(), MeshShapes::torus(0.8f, 0.25f, 96, 32),
        std::string("torus"));

    const auto addViewport = [&](std::string_view name, glm::vec3 eye) {
        auto viewport = root.branch(name);
        auto camera = viewport.emplace<Camera>("camera");
        camera->setPosition(eye);
        camera.param("position", camera->transform().position);
        auto view = viewport.construct<Viewport>(render->layout(), camera);
        view.param("ambient", &Viewport::ambient);
        auto controller = viewport.addSlot<TransformController>("controller");
        controller.choice("Graphics::FreeCamera", composeFreeCamera);
        viewport.emplace<Spheres, SceneObject>("particles", render->layout(), particles,
            SphereColumns{
                .x = std::string(Lattice::typeName<Pos::X>()),
                .y = std::string(Lattice::typeName<Pos::Y>()),
                .z = std::string(Lattice::typeName<Pos::Z>()),
            },
            std::filesystem::path("packages/graphics/shaders/spheres3d.wgsl"));
        auto wallsView = viewport.emplace<MeshObject, SceneObject>("walls", render->layout(), walls);
        wallsView->color = {0.55f, 0.65f, 0.75f};
        exportMesh(wallsView);
        auto mesh = viewport.emplace<MeshObject, SceneObject>("torus", render->layout(), torus);
        mesh->setPosition({3.2f, 0.0f, 0.0f});
        mesh->display = "shaded-edges";
        exportMesh(mesh);
        addLight(viewport, "sun", Light{.color = {1.0f, 0.96f, 0.9f}, .intensity = 0.9f, .direction = {-0.4f, -1.0f, -0.6f}});
        addLight(viewport, "fill", Light{.color = {0.6f, 0.7f, 1.0f}, .intensity = 0.35f, .direction = {0.7f, 0.2f, 0.5f}});
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
// its scene objects under its lights and is driven by the controller below it.
extern "C" void configure(NodeConfigure root) {
    root.self<Render>()->setViewports(root.collect<Viewport>());
    for (auto [viewport, node] : root.collectNodes<Viewport>()) {
        viewport->setObjects(node.children<SceneObject>());
        viewport->setLights(node.children<Light>());
        const auto controllers = node.children<TransformController>();
        if (controllers.size() > 1)
            throw Lattice::Exception<Viewport>("Viewport #{} has {} controllers", node.id(), controllers.size());
        viewport->controller = controllers.empty() ? nullptr : controllers.front();
    }
}
