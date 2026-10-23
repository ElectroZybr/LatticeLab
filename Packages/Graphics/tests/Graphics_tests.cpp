// Camera math and controllers without GPU; TestViewport only when an adapter exists.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <Graphics/Camera.hpp>
#include <Graphics/TestViewport.hpp>
#include <Graphics/TransformMode.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct NullViewport final : Viewport {
    void render(WGPU::RenderPass&, glm::uvec2) override {}
};

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
    NullViewport viewport;
    FreeCameraController controller{camera, viewport};
    controller.move = {0, 0, 1};
    TransformController& generic = controller;
    generic.update();
    check(camera.transform().position.z < 0.0f, "moved forward along -Z");
    check(controller.move == glm::vec3{}, "input is consumed per frame");
}

void orbitControllerKeepsDistance() {
    Camera camera;
    camera.setPosition({0, 0, 5});
    NullViewport viewport;
    OrbitCameraController controller{camera, viewport};
    controller.orbit = {100, 0};
    controller.update();
    check(std::abs(glm::length(camera.transform().position) - 5.0f) < 1e-3f, "orbit keeps the distance to the pivot");
}

bool testViewportOnDevice() {
    WGPU::Instance instance;
    try {
        WGPU::Device device{instance};
        Camera camera;
        {
            TestViewport viewport{device, WGPU::TextureFormat::BGRA8Unorm, camera, GRAPHICS_SHADERS "/circle.wgsl"};
            check(device.resources().size() == 4, "viewport owns buffer, shader, pipeline and bindings");
        }
        check(device.resources().size() == 0, "viewport releases its resources");
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
    orbitControllerKeepsDistance();
    const bool gpu = testViewportOnDevice();
    std::cout << "Graphics: camera, controllers" << (gpu ? " and TestViewport resources" : "") << " passed\n";
}
