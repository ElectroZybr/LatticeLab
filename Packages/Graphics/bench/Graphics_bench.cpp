// Frame costs of scene objects on the GPU: run `xmake run Graphics.bench [name or group]`.
// Mesh: a lit torus of N triangles at 1920×1080 — solid, with the wireframe, transparent.
// N is the number of pixels of the frame (16:9); a frame is recorded, submitted and waited
// for, its GPU time comes from timestamps of the render pass (BmRunner Trace).
// GeometrySurface: the demo container from different camera positions — the cost of
// sphere tracing grows with the covered pixels and with steps near surfaces. Each view is
// a work with two implementations of the distance: the interpreting machine and code
// generated for the program; generated frames are checked against interpreted ones.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <BmRunner/Benchmarks.hpp>
#include <BmRunner/Main.hpp>
#include <Boundaries/Shape.hpp>
#include <Boundaries/Space.hpp>
#include <BoundariesWgpu/Codegen.hpp>
#include <BoundariesWgpu/GpuGeometry.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/GeometrySurface.hpp>
#include <Graphics/GpuMesh.hpp>
#include <Graphics/Light.hpp>
#include <Graphics/MeshObject.hpp>
#include <Graphics/MeshShapes.hpp>
#include <Graphics/RenderLayout.hpp>
#include <Graphics/RenderPass.hpp>
#include <Graphics/Viewport.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/Timer.hpp>

using Lattice::Benchmarks::Phase;
using Lattice::Benchmarks::Run;
using Lattice::Benchmarks::Trace;

namespace {

/// One device for the whole run.
Wgpu::Device& device() {
    static Wgpu::Instance instance;
    static Wgpu::Device device{instance};
    return device;
}

/// 480×270, 960×540, 1920×1080, 3840×2160.
const std::vector<size_t> Pixels = {129'600, 518'400, 2'073'600, 8'294'400};

glm::uvec2 resolution(size_t pixels) {
    const auto width = uint32_t(std::lround(std::sqrt(double(pixels) * 16.0 / 9.0)));
    return {width, uint32_t(pixels / width)};
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

/// RGBA8 pixels of a frame, row after row.
struct Image {
    glm::uvec2 size;
    std::vector<uint8_t> rgba;
};

/// A frame of N pixels seen from `eye`: the input of a view work; as an output, also the
/// frame drawn.
struct View {
    size_t pixels;
    glm::vec3 eye;
    Image image;
};

/// Frames differ by at most 8 levels per channel except on 0.5% of the pixels: the
/// distances of the two implementations round differently, which moves silhouettes by a
/// pixel here and there.
bool near(const View& expectedView, const View& actualView) {
    const Image& expected = expectedView.image;
    const Image& actual = actualView.image;
    if (expected.size != actual.size || expected.rgba.size() != actual.rgba.size())
        return false;
    size_t differing = 0;
    for (size_t i = 0; i < expected.rgba.size(); i += 4)
        for (size_t c = 0; c < 4; ++c)
            if (std::abs(int(expected.rgba[i + c]) - int(actual.rgba[i + c])) > 8) {
                ++differing;
                break;
            }
    return differing * 200 <= expected.rgba.size() / 4;
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

    /// The last frame, read back (rows of the copy are padded to 256 bytes).
    Image image() {
        const uint32_t row = size.x * 4;
        const uint32_t padded = (row + Wgpu::CommandList::RowAlignment - 1) / Wgpu::CommandList::RowAlignment *
            Wgpu::CommandList::RowAlignment;
        Wgpu::Buffer staging{device(), {.size = uint64_t(padded) * size.y,
            .usage = Wgpu::BufferUsage::CopySource | Wgpu::BufferUsage::CopyDestination, .label = "bench/image"}};
        Wgpu::CommandList commands{device()};
        commands.copyTexture(color, staging, padded);
        commands.submit();
        const auto bytes = device().readBuffer(staging, 0, staging.size());
        Image result{size, std::vector<uint8_t>(size_t(row) * size.y)};
        for (uint32_t y = 0; y < size.y; ++y)
            std::memcpy(result.rgba.data() + size_t(y) * row, bytes.data() + size_t(y) * padded, row);
        return result;
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

/// A frame target of N pixels drawing the container; one per point N.
struct Frame : Lattice::Fixture {
    using Input = View;
    static constexpr bool Reuse = true;
    Frame(const View& view, Boundaries::Wgpu::DistanceSource distance)
        : pixels(view.pixels),
          target(resolution(view.pixels)),
          space(glm::vec3(-2.0f), glm::vec3(4.0f)),
          geometry(device(), container()),
          surface(target.layout, geometry, space, distance) {
        target.camera.setPosition(view.eye);
        target.viewport.setObjects({&surface});
    }

    size_t pixels;
    Target target;
    Boundaries::Space space;
    Boundaries::Wgpu::GpuGeometry geometry;
    GeometrySurface surface;
};

struct InterpretedFrame : Frame {
    explicit InterpretedFrame(const View& view) : Frame(view, Boundaries::Wgpu::interpretedDistance) {}
};

struct GeneratedFrame : Frame {
    explicit GeneratedFrame(const View& view) : Frame(view, Boundaries::Wgpu::generateDistance) {}
};

/// An implementation of a view work: draw frames, output the last one.
template<class Implementation>
void frames(Implementation& impl) {
    impl.output([](Frame& frame) {
        return View{frame.pixels, frame.target.camera.transform().position, frame.target.image()};
    });
    impl.measure([](Frame& frame, Run& run) { frame.target.draw(run.trace()); });
}

}

BENCH_GROUP(GeometrySurface) { bench.config.sizes = Pixels; }

#define VIEW_WORK(Name, eye, description)                                                       \
    BENCH_WORK(GeometrySurface, Name, View, description) {                                    \
        work.input([](size_t pixels) { return View{pixels, eye, {}}; });                          \
        work.check(near);                                                                     \
        work.reference("interpreter");                                                        \
    }                                                                                         \
    BENCH_IMPLEMENTATION(GeometrySurface, Name, "interpreter", InterpretedFrame) { frames(impl); } \
    BENCH_IMPLEMENTATION(GeometrySurface, Name, "codegen", GeneratedFrame) { frames(impl); }

VIEW_WORK(Far, glm::vec3(0.0f, 0.0f, 5.0f), "the whole container in view, outside the box")
VIEW_WORK(NearWall, glm::vec3(0.0f, 0.0f, 1.9f), "inside the box, in the wall next to the cut-away side")
VIEW_WORK(Inside, glm::vec3(0.0f, 0.0f, 0.9f), "in the region, 0.4 in front of the obstacle")

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

BENCH_GROUPED(Mesh, Wireframe, MeshFrame, "the same torus with the wireframe over it") {
    bench.measure<MeshFrame>([](MeshFrame& frame, Trace& trace) {
        frame.object.wireframe = true;
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
