// Camera math and controllers without GPU; scene resources only when an adapter exists.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <Graphics/Camera.hpp>
#include <Graphics/RenderLayout.hpp>
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

bool sceneResourcesOnDevice() {
    WGPU::Instance instance;
    try {
        WGPU::Device device{instance};
        const auto errors = WGPU::errorCount();
        {
            RenderLayout layout{device, WGPU::TextureFormat::BGRA8Unorm};
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
        check(WGPU::errorCount() == errors, "object pipeline matches the shared camera layout");
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
    std::cout << "Graphics: camera, controllers" << (gpu ? " and scene resources" : "") << " passed\n";
}
