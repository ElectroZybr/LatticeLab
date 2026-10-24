// Camera math and controllers without GPU; scene resources only when an adapter exists.
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <Graphics/Camera.hpp>
#include <Graphics/RenderLayout.hpp>
#include <Graphics/RenderPipeline.hpp>
#include <Graphics/Spheres.hpp>
#include <Graphics/TestObject.hpp>
#include <Graphics/Viewport.hpp>
#include <Graphics/TransformMode.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void cameraProjectsCenterRayForward() {
    Camera camera;
    camera.setPosition({0, 0, 2});
    const Ray ray = camera.screenRay({640, 360}, {1280, 720});
    check(std::abs(ray.direction.z + 1.0f) < 1e-3f, "center ray looks down -Z");
    const glm::vec3 hit = camera.screenToPlane({640, 360}, {1280, 720});
    check(glm::length(hit) < 1e-3f, "center hits the origin plane");
}

void freeControllerConsumesInput() {
    Camera camera;
    ViewportArea viewport;
    FreeCameraController controller{camera, viewport};
    controller.move = {0, 0, 1};
    TransformController& generic = controller;
    generic.update();
    check(camera.transform().position.z < 0.0f, "moved forward along -Z");
    check(controller.move == glm::vec3{}, "input is consumed per frame");
}

void viewportHitTestAndLocalCursor() {
    ViewportArea panel;
    panel.setPosition({100, 50});
    panel.setSize({200, 100});
    check(panel.contains({150, 60}, {1280, 720}), "inside the panel");
    check(!panel.contains({50, 60}, {1280, 720}) && !panel.contains({150, 200}, {1280, 720}), "outside the panel");
    panel.hover(glm::vec2{50, 10});
    check(panel.hovered() && panel.cursor() == glm::vec2{50, 10}, "local cursor while hovered");
    panel.hover(std::nullopt);
    check(!panel.hovered(), "cursor left the panel");
}

void orbitControllerKeepsDistance() {
    Camera camera;
    camera.setPosition({0, 0, 5});
    ViewportArea viewport;
    OrbitCameraController controller{camera, viewport};
    controller.look = {100, 0};
    controller.update();
    check(std::abs(glm::length(camera.transform().position) - 5.0f) < 1e-3f, "orbit keeps the distance to the pivot");
}

const char* layoutShader = R"(
struct Camera { viewProjection : mat4x4<f32> }
@group(0) @binding(0) var<uniform> camera : Camera;
@group(1) @binding(0) var<storage, read> data : array<f32>;
@vertex fn vs(@builtin(vertex_index) i : u32, @builtin(instance_index) n : u32) -> @builtin(position) vec4<f32> {
    return camera.viewProjection * vec4<f32>(data[n], f32(i), 0.0, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return vec4<f32>(1.0); }
)";

void pipelinesShareTheCameraLayout(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        const std::array<Wgpu::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
        const std::array<Wgpu::LayoutEntry, 1> dataEntries{{{.binding = 0,
            .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex}}};
        Wgpu::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        Wgpu::BindingLayout dataLayout{device, dataEntries, "data"};
        Wgpu::Shader shader{device, {.source = layoutShader, .label = "layouts"}};
        // Two pipelines on the same camera layout: one bind group serves both.
        RenderPipeline flat{device, shader, {.layouts = {&cameraLayout, &dataLayout}, .label = "flat"}};
        RenderPipeline deep{device, shader, {.depthFormat = Wgpu::TextureFormat::Depth32Float,
            .layouts = {&cameraLayout, &dataLayout}, .label = "deep"}};
    }
    check(Wgpu::errorCount() == errors, "pipelines with and without depth on one camera layout");
    {
        const std::array<Wgpu::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
        Wgpu::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        Wgpu::Shader shader{device, {.source = layoutShader, .label = "layouts"}};
        try {
            RenderPipeline missing{device, shader, {.layouts = {&cameraLayout}, .label = "missing group 1"}};
        } catch (const Lattice::ExceptionBase&) {}
    }
    check(Wgpu::errorCount() > errors, "a pipeline without the data group is reported");
}

bool sceneResourcesOnDevice() {
    Wgpu::Instance instance;
    try {
        Wgpu::Device device{instance};
        const auto errors = Wgpu::errorCount();
        {
            RenderLayout layout{device, Wgpu::TextureFormat::BGRA8Unorm};
            Camera camera;
            Viewport viewport{layout, camera};
            TestObject marker{layout, GRAPHICS_SHADERS "/circle.wgsl"};
            viewport.setObjects({&marker});
            check(viewport.objects().size() == 1, "viewport draws its objects");
            // layout (camera group) + viewport (uniform, group) + object (layout, uniform,
            // group, shader, pipeline)
            check(device.resources().size() == 8, "scene owns its GPU resources");
        }
        check(device.resources().size() == 0, "scene releases its resources");
        {
            RenderLayout layout{device, Wgpu::TextureFormat::BGRA8Unorm};
            Wgpu::GpuSoA table{device, "particles"};
            Spheres spheres{layout, table, {.x = "x", .y = "y", .z = "z"}, GRAPHICS_SHADERS "/spheres3d.wgsl"};
            spheres.setPalette({{1, 0, 0, 0.5f}, {0, 1, 0, 0.25f}});
            check(spheres.count() == 0, "nothing drawn before a frame");
        }
        check(device.resources().size() == 0, "spheres release their resources");
        check(Wgpu::errorCount() == errors, "object pipeline matches the shared camera layout");
        pipelinesShareTheCameraLayout(device);
        return true;
    } catch (const Lattice::ExceptionBase& error) {
        std::cout << "Graphics: GPU checks skipped (" << error.what() << ")\n";
        return false;
    }
}
}

int main() {
    cameraProjectsCenterRayForward();
    freeControllerConsumesInput();
    viewportHitTestAndLocalCursor();
    orbitControllerKeepsDistance();
    const bool gpu = sceneResourcesOnDevice();
    std::cout << "Graphics: camera, controllers" << (gpu ? " and scene/spheres resources" : "") << " passed\n";
}
