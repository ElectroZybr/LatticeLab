// Camera math and controllers without GPU; scene resources only when an adapter exists.
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <tuple>
#include <cmath>
#include <iostream>
#include <span>
#include <stdexcept>
#include <Boundaries/Shape.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/GpuMesh.hpp>
#include <Graphics/MeshObject.hpp>
#include <Graphics/MeshShapes.hpp>
#include <Graphics/RenderPass.hpp>
#include <Graphics/SdfMesh.hpp>
#include <Graphics/SurfaceNets.hpp>
#include <BoundariesCpu/Geometry.hpp>
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

/// Closed and consistently oriented: welded by position, every edge is walked as often in
/// each direction (surface nets may share an edge among four faces); faces turn the way
/// their vertex normals point; normals are unit.
void checkClosed(const MeshData& mesh, const char* name) {
    std::map<std::tuple<long, long, long>, uint32_t> welded;
    std::vector<uint32_t> id(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const glm::vec3 p = mesh.vertices[i].position * 1e4f;
        id[i] = welded.try_emplace({std::lround(p.x), std::lround(p.y), std::lround(p.z)}, uint32_t(welded.size()))
            .first->second;
        check(std::abs(glm::length(mesh.vertices[i].normal) - 1.0f) < 1e-4f, name);
    }
    std::map<std::pair<uint32_t, uint32_t>, int> edges;
    for (size_t t = 0; t < mesh.indices.size(); t += 3) {
        const auto& a = mesh.vertices[mesh.indices[t]];
        const auto& b = mesh.vertices[mesh.indices[t + 1]];
        const auto& c = mesh.vertices[mesh.indices[t + 2]];
        check(glm::dot(glm::cross(b.position - a.position, c.position - a.position), a.normal + b.normal + c.normal) > 0,
              name);
        for (int k = 0; k < 3; ++k)
            ++edges[{id[mesh.indices[t + k]], id[mesh.indices[t + (k + 1) % 3]]}];
    }
    for (const auto& [edge, count] : edges)
        check(edges.contains({edge.second, edge.first}) && edges.at({edge.second, edge.first}) == count, name);
}

void meshShapesAreClosed() {
    checkClosed(MeshShapes::box({1.0f, 0.5f, 0.25f}), "box is closed and outward");
    checkClosed(MeshShapes::sphere(1.0f, 24, 12), "sphere is closed and outward");
    const MeshData torus = MeshShapes::torus(1.0f, 0.3f, 32, 12);
    checkClosed(torus, "torus is closed and outward");
    check(torus.triangles() == 2 * 32 * 12, "torus has 2 · segments · sides triangles");
}

void surfaceNetsOfSphereAndPlane() {
    const SurfaceGrid grid{.origin = glm::vec3(-1.0f), .cellSize = glm::vec3(0.0625f), .cells = glm::uvec3(32)};
    const auto sphere = [](glm::vec3 p) { return 0.7f - glm::length(p - glm::vec3(0.05f, -0.02f, 0.03f)); };
    const MeshData mesh = surfaceNets(grid, sampleGrid(grid, sphere));
    check(!mesh.indices.empty(), "the sphere has a surface");
    checkClosed(mesh, "the sphere net is closed and faces the region");
    float worst = 0.0f;
    for (const MeshVertex& vertex : mesh.vertices) {
        worst = std::max(worst, std::abs(sphere(vertex.position)));
        check(glm::dot(vertex.normal, glm::vec3(0.05f, -0.02f, 0.03f) - vertex.position) > 0.0f, "normals point inward");
    }
    check(worst < 0.2f * 0.0625f, "sphere net vertices lie on the surface");

    const MeshData plane = surfaceNets(grid, sampleGrid(grid, [](glm::vec3 p) { return p.y - 0.3f; }));
    check(plane.triangles() == 2 * 31 * 31, "a plane through the grid: a quad per inner crossed edge");
    for (const MeshVertex& vertex : plane.vertices)
        check(std::abs(vertex.position.y - 0.3f) < 1e-5f && vertex.normal.y > 0.999f, "plane net is flat and faces +y");
}

/// A frame of 64×64 into a texture, read back as RGBA8.
std::vector<uint8_t> drawFrame(Wgpu::Device& device, RenderLayout& layout, Viewport& viewport) {
    constexpr uint32_t size = 64, row = Wgpu::CommandList::RowAlignment;
    Wgpu::Texture color{device, {.width = size, .height = size, .format = Wgpu::TextureFormat::RGBA8Unorm,
        .usage = Wgpu::TextureUsage::RenderAttachment | Wgpu::TextureUsage::CopySource, .label = "test/color"}};
    Wgpu::Texture depth{device, {.width = size, .height = size, .format = layout.depthFormat(),
        .usage = Wgpu::TextureUsage::RenderAttachment, .label = "test/depth"}};
    Wgpu::Buffer staging{device, {.size = row * size,
        .usage = Wgpu::BufferUsage::CopySource | Wgpu::BufferUsage::CopyDestination, .label = "test/frame"}};
    Wgpu::CommandList commands{device};
    {
        RenderPass pass{commands, color, {0, 0, 0, 1}, &depth};
        viewport.render(pass, {size, size});
    }
    commands.copyTexture(color, staging, row);
    commands.submit();
    const auto bytes = device.readBuffer(staging, 0, staging.size());
    std::vector<uint8_t> pixels(size * size * 4);
    for (uint32_t y = 0; y < size; ++y)
        std::memcpy(pixels.data() + y * size * 4, bytes.data() + y * row, size * 4);
    return pixels;
}

/// A lit sphere in front of the camera covers the center and leaves the corners clear;
/// every display mode builds its pipeline without validation errors.
void meshIsDrawnAndLit(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        RenderLayout layout{device, Wgpu::TextureFormat::RGBA8Unorm};
        Camera camera;
        camera.setPosition({0, 0, 3});
        Viewport viewport{layout, camera};
        GpuMesh mesh{device, MeshShapes::sphere(1.0f, 32, 16), "sphere"};
        MeshObject object{layout, mesh};
        Light light{.direction = {0, 0, -1}};
        viewport.setObjects({&object});
        viewport.setLights({&light});

        auto pixels = drawFrame(device, layout, viewport);
        const auto at = [&](uint32_t x, uint32_t y) { return pixels[(y * 64 + x) * 4]; };
        check(at(32, 32) > 100, "the lit sphere covers the center");
        check(at(1, 1) == 0, "corners stay clear");
        viewport.setLights({});
        pixels = drawFrame(device, layout, viewport);
        check(at(32, 32) < 50, "without lights only the ambient light is left");

        object.wireframe = true;
        object.opacity = 0.5f;
        object.cullBack = false;
        drawFrame(device, layout, viewport);
        object.solid = false;
        drawFrame(device, layout, viewport);
    }
    check(device.resources().size() == 0, "mesh resources are released");
    check(Wgpu::errorCount() == errors, "mesh pipelines in every mode match the layouts");
}

MeshData readMesh(Wgpu::Device& device, const SdfMesh& mesh) {
    MeshData data;
    data.vertices.resize(mesh.vertexCount());
    data.indices.resize(mesh.indexCount());
    device.readBuffer(*mesh.vertices(), 0, std::as_writable_bytes(std::span(data.vertices)));
    device.readBuffer(*mesh.indices(), 0, std::as_writable_bytes(std::span(data.indices)));
    return data;
}

/// The GPU net in bricks against the CPU net of the same grid. Sizes stay off the grid
/// planes: a surface through corners leaves their sign to rounding, which differs.
void sdfMeshMatchesTheCpu(Wgpu::Device& device) {
    using Boundaries::Shape;
    const auto errors = Wgpu::errorCount();
    {
        const Shape shape = Shape::cylinder(0.53f, 0.57f).rounded(0.17f) - Shape::sphere(0.31f);
        const Boundaries::Space space{glm::vec3(-1.0f), glm::vec3(2.0f)};
        Boundaries::Wgpu::GpuGeometry geometry{device, shape.compile()};
        SdfMesh mesh{device, geometry, space, 48};
        const Boundaries::Cpu::Geometry cpu{shape};
        const SurfaceGrid grid = mesh.grid();
        check(grid.cells == glm::uvec3(48), "the grid covers the space in whole bricks");
        const MeshData expected = surfaceNets(grid, sampleGrid(grid, [&](glm::vec3 p) { return cpu.distance(p); }));
        const MeshData actual = readMesh(device, mesh);
        checkClosed(actual, "the GPU net is closed and faces the region");

        const auto cellOf = [&](glm::vec3 p) {
            const glm::uvec3 c = glm::uvec3((p - grid.origin) / grid.cellSize);
            return (size_t(c.z) * grid.cells.y + c.y) * grid.cells.x + c.x;
        };
        std::map<size_t, glm::vec3> cpuVertices;
        for (const MeshVertex& vertex : expected.vertices)
            cpuVertices[cellOf(vertex.position)] = vertex.position;
        size_t matched = 0;
        for (const MeshVertex& vertex : actual.vertices) {
            const auto found = cpuVertices.find(cellOf(vertex.position));
            if (found != cpuVertices.end() && glm::length(found->second - vertex.position) < 1e-3f * grid.cellSize.x)
                ++matched;
        }
        check(matched * 1000 >= expected.vertices.size() * 998 && actual.vertices.size() * 1000 <= expected.vertices.size() * 1002,
              "GPU vertices are the CPU ones");
        check(actual.indices.size() * 1000 >= expected.indices.size() * 995 &&
              actual.indices.size() * 1000 <= expected.indices.size() * 1005, "GPU faces are the CPU ones");
        check(mesh.activeBricks() < 6 * 6 * 6, "empty bricks are skipped");

        check(!mesh.update(), "nothing changed, nothing rebuilt");
        geometry.upload((Shape::cylinder(0.37f, 0.57f).rounded(0.17f) - Shape::sphere(0.21f)).compile());
        const uint32_t before = mesh.indexCount();
        check(mesh.update() && mesh.indexCount() < before, "new parameters rebuild the net");
        mesh.resolution = 96;
        check(mesh.update() && mesh.indexCount() > 2 * before, "a finer grid rebuilds the net");
    }
    check(device.resources().size() == 0, "the SDF mesh releases its resources");
    check(Wgpu::errorCount() == errors, "surface nets passes are valid");
}

constexpr unsigned char LayoutShader[] = {
#include "layouts.wgsl.h"
};

void pipelinesShareTheCameraLayout(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        const std::array<Wgpu::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
        const std::array<Wgpu::LayoutEntry, 1> dataEntries{{{.binding = 0,
            .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex}}};
        Wgpu::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        Wgpu::BindingLayout dataLayout{device, dataEntries, "data"};
        Wgpu::Shader shader{device, {.source = reinterpret_cast<const char*>(LayoutShader), .label = "layouts"}};
        // Two pipelines on the same camera layout: one bind group serves both.
        RenderPipeline flat{device, shader, {.layouts = {&cameraLayout, &dataLayout}, .label = "flat"}};
        RenderPipeline deep{device, shader, {.depthFormat = Wgpu::TextureFormat::Depth32Float,
            .layouts = {&cameraLayout, &dataLayout}, .label = "deep"}};
    }
    check(Wgpu::errorCount() == errors, "pipelines with and without depth on one camera layout");
    {
        const std::array<Wgpu::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
        Wgpu::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        Wgpu::Shader shader{device, {.source = reinterpret_cast<const char*>(LayoutShader), .label = "layouts"}};
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
            // layout (camera group) + viewport (camera, lights, group) + object (layout,
            // uniform, group, shader, pipeline)
            check(device.resources().size() == 9, "scene owns its GPU resources");
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
        meshIsDrawnAndLit(device);
        sdfMeshMatchesTheCpu(device);
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
    meshShapesAreClosed();
    surfaceNetsOfSphereAndPlane();
    const bool gpu = sceneResourcesOnDevice();
    std::cout << "Graphics: camera, controllers, mesh shapes, surface nets" << (gpu ? " and scene/spheres/mesh/SDF mesh resources" : "") << " passed\n";
}
