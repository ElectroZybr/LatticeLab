#include <filesystem>
#include <string>
#include <string_view>

#include <glm/vec3.hpp>
#include <Lattice/Composition.hpp>
#include <Actions/InputTargetAPI.hpp>
#include <Boundaries/Shape.hpp>
#include <Boundaries/Space.hpp>
#include <BoundariesWgpu/GpuGeometry.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/GpuMesh.hpp>
#include <Graphics/Light.hpp>
#include <Graphics/MeshObject.hpp>
#include <Graphics/MeshShapes.hpp>
#include <Graphics/SdfMesh.hpp>
#include <Graphics/Render.hpp>
#include <Graphics/Spheres.hpp>
#include <Graphics/TransformMode.hpp>
#include <Graphics/Viewport.hpp>
#include <ParticleDynamics/ParticleSystemAPI.hpp>
#include <Shell/WindowAPI.hpp>
#include <Wgpu/GpuSoA.hpp>

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
    mesh.param("color", &MeshObject::color);
    mesh.param("opacity", &MeshObject::opacity);
    mesh.param("specular", &MeshObject::specular);
    mesh.param("shininess", &MeshObject::shininess);
    mesh.param("solid", &MeshObject::solid);
    mesh.param("wireframe", &MeshObject::wireframe);
    mesh.param("wireColor", &MeshObject::wireColor);
    mesh.param("wireWidth", &MeshObject::wireWidth);
    mesh.param("cullBack", &MeshObject::cullBack);
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

// Demo geometry until the simulation publishes its own: a rounded cylindrical container
// with an obstacle (a sphere smoothly joined with a bar) in the middle.
Boundaries::Program demoGeometry() {
    using Boundaries::Shape;
    const auto container = Shape::cylinder(1.4f, 1.4f).rounded(0.3f);
    const auto obstacle = Shape::smoothUnion(Shape::sphere(0.5f),
        Shape::capsule({-1.2f, 0.0f, 0.0f}, {1.2f, 0.0f, 0.0f}, 0.15f), 0.25f);
    return (container - obstacle).compile();
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

    // Geometry on the device and the box it is drawn in (hardcoded for now).
    auto space = root.emplace<Boundaries::Space>("space", glm::vec3(-2.0f), glm::vec3(4.0f));
    auto geometry = root.emplace<Boundaries::Wgpu::GpuGeometry>("geometry", device.ref(), demoGeometry());
    auto walls = root.emplace<SdfMesh>("walls", device.ref(), geometry, space, 128u);
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
            std::filesystem::path("Packages/Graphics/shaders/spheres3d.wgsl"));
        auto wallsView = viewport.emplace<MeshObject, SceneObject>("walls", render->layout(), walls);
        wallsView->color = {0.55f, 0.65f, 0.75f};
        exportMesh(wallsView);
        auto mesh = viewport.emplace<MeshObject, SceneObject>("torus", render->layout(), torus);
        mesh->setPosition({3.2f, 0.0f, 0.0f});
        mesh->wireframe = true;
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
