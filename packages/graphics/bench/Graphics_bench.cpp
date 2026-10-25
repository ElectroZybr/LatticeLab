// Frame costs of scene objects on the GPU: `xmake run Graphics.bench [name or group]`.
// A frame is recorded, submitted and waited for; its GPU time comes from timestamps of the
// render pass. Mesh: a lit torus of N triangles. SdfMesh: surface nets of the demo
// container with N cells along the box, its build and a frame near its wall.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <lbm-runner/Benchmarks.hpp>
#include <lbm-runner/Main.hpp>
#include <boundaries/Shape.hpp>
#include <boundaries/Space.hpp>
#include <graphics/Camera.hpp>
#include <graphics/GpuBoundaries.hpp>
#include <graphics/GpuMesh.hpp>
#include <graphics/Light.hpp>
#include <graphics/MeshObject.hpp>
#include <graphics/MeshShapes.hpp>
#include <graphics/SdfMesh.hpp>
#include <graphics/RenderLayout.hpp>
#include <graphics/RenderPass.hpp>
#include <graphics/Viewport.hpp>
#include <wgpu/CommandList.hpp>
#include <wgpu/Device.hpp>
#include <wgpu/Timer.hpp>

using Lattice::Benchmarks::Phase;
using Lattice::Benchmarks::Trace;

namespace {

/// One device for the whole run.
Wgpu::Device& device() {
    static Wgpu::Instance instance;
    static Wgpu::Device device{instance};
    return device;
}

/// The container drawn by the Graphics demo: a rounded cylinder minus a sphere joined
/// with a bar, in the box [-2, 2]³.
Boundaries::Program container() {
    using Boundaries::Shape;
    const auto vessel = Shape::cylinder(1.4f, 1.4f).rounded(0.3f);
    const auto obstacle = Shape::smoothUnion(Shape::sphere(0.5f),
        Shape::capsule({-1.2f, 0.0f, 0.0f}, {1.2f, 0.0f, 0.0f}, 0.15f), 0.25f);
    return (vessel - obstacle).compile();
}

/// A frame target (color, depth) with one viewport, its camera and lights; a frame is
/// recorded, submitted and waited for, the pass time comes from the timer.
struct Target {
    explicit Target(glm::uvec2 size)
        : size(size),
          layout(device(), Wgpu::TextureFormat::RGBA8Unorm),
          color(device(), {.width = size.x, .height = size.y, .format = Wgpu::TextureFormat::RGBA8Unorm,
              .usage = Wgpu::TextureUsage::RenderAttachment | Wgpu::TextureUsage::CopySource, .label = "bench/color"}),
          depth(device(), {.width = size.x, .height = size.y, .format = Wgpu::TextureFormat::Depth32Float,
              .usage = Wgpu::TextureUsage::RenderAttachment, .label = "bench/depth"}),
          viewport(layout, camera),
          timer(device(), 1024, "bench/frame") {
        viewport.setLights({&sun, &fill});
    }

    void draw(Trace& trace) {
        Wgpu::CommandList commands{device()};
        {
            auto prepare = trace.span(Phase::Prepare);
            RenderPass pass{commands, color, {0, 0, 0, 1}, &depth, timer.renderPass("frame")};
            viewport.render(pass, size);
        }
        {
            auto issue = trace.span(Phase::Issue);
            commands.submit();
        }
        {
            auto complete = trace.span(Phase::Complete);
            device().wait();
        }
        trace.deferIntervals(timer);
    }

    glm::uvec2 size;
    RenderLayout layout;
    Wgpu::Texture color, depth;
    Camera camera;
    Light sun{.direction = {-0.4f, -1.0f, -0.6f}};
    Light fill{.intensity = 0.35f, .direction = {0.7f, 0.2f, 0.5f}};
    Viewport viewport;
    Wgpu::Timer timer;
};

}

namespace {

/// A torus of N triangles (√N segments and sides) filling a 1920×1080 frame; one per point N.
struct MeshFrame : Lattice::Fixture {
    static constexpr bool Reuse = true;
    explicit MeshFrame(size_t triangles)
        : target({1920, 1080}),
          mesh(device(), torus(triangles), "bench/torus"),
          object(target.layout, mesh) {
        target.camera.setPosition({0.0f, 0.6f, 2.4f});
        target.viewport.setObjects({&object});
    }

    static MeshData torus(size_t triangles) {
        const auto sides = uint32_t(std::max(3.0, std::round(std::sqrt(double(triangles) / 2.0))));
        return MeshShapes::torus(0.8f, 0.3f, sides, sides);
    }

    Target target;
    GpuMesh mesh;
    MeshObject object;
};

}

BENCH_GROUP(Mesh) { bench.config.sizes = {10'000, 100'000, 1'000'000, 4'000'000}; }

BENCH_GROUPED(Mesh, Solid, MeshFrame, "lit torus of N triangles, 1920×1080") {
    bench.measure<MeshFrame>([](MeshFrame& frame, Trace& trace) { frame.target.draw(trace); });
}

BENCH_GROUPED(Mesh, Triangles, MeshFrame, "the same torus with its triangles over it") {
    bench.measure<MeshFrame>([](MeshFrame& frame, Trace& trace) {
        frame.object.triangles = true;
        frame.target.draw(trace);
    });
}

BENCH_GROUPED(Mesh, HiddenVisible, MeshFrame, "the same torus, visible and dashed hidden lines") {
    bench.measure<MeshFrame>([](MeshFrame& frame, Trace& trace) {
        frame.object.display = "hidden-visible";
        frame.target.draw(trace);
    });
}

BENCH_GROUPED(Mesh, Transparent, MeshFrame, "the same torus at half opacity, both sides") {
    bench.measure<MeshFrame>([](MeshFrame& frame, Trace& trace) {
        frame.object.opacity = 0.5f;
        frame.object.cullBack = false;
        frame.target.draw(trace);
    });
}

namespace {

struct ContainerMesh : Lattice::Fixture {
    static constexpr bool Reuse = true;
    explicit ContainerMesh(size_t resolution)
        : target({1920, 1080}),
          boundaries(device()),
          mesh(device(), boundaries, uint32_t(resolution)),
          object(target.layout, mesh) {
        boundaries.set(Boundaries::Space(glm::vec3(-2.0f), glm::vec3(4.0f)), container());
        mesh.build();
        target.viewport.setObjects({&object});
    }

    Target target;
    GpuBoundaries boundaries;
    SdfMesh mesh;
    MeshObject object;
};

}

BENCH_GROUP(SdfMesh) { bench.config.sizes = {64, 128, 256, 512}; }

BENCH_GROUPED(SdfMesh, Build, ContainerMesh, "surface nets of the container, N cells along the box") {
    bench.measure<ContainerMesh>([](ContainerMesh& fixture, Trace& trace) {
        auto complete = trace.span(Phase::Complete);
        fixture.mesh.build();
    });
}

BENCH_GROUPED(SdfMesh, FrameNearWall, ContainerMesh, "the container mesh at 1920×1080, the camera next to the wall") {
    bench.measure<ContainerMesh>([](ContainerMesh& fixture, Trace& trace) {
        fixture.target.camera.setPosition({0.0f, 0.0f, 1.9f});
        fixture.target.draw(trace);
    });
}

// [filter] [--filter <group|bench|mask>] [--tsv <file>] [--list]; a group run compares
// the implementations of each view.
int main(int argc, char** argv) {
    const int status = Lattice::Benchmarks::main(argc, argv);
    if (Wgpu::errorCount()) {
        std::cerr << "Graphics.bench: " << Wgpu::errorCount() << " GPU validation errors\n";
        return 1;
    }
    return status;
}
