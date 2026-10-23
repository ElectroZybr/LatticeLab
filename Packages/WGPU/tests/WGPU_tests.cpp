// Registry logic always; device and resources only when an adapter is available.
#include <array>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <WGPU/CommandList.hpp>
#include <WGPU/Device.hpp>
#include <WGPU/GpuSoA.hpp>
#include <WGPU/Resources.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void registryTracksLiveResources() {
    WGPU::ResourceRegistry registry;
    const auto a = registry.add("Buffer", "uniform", 64);
    const auto b = registry.add("Shader", "scene", 0);
    check(registry.size() == 2 && registry.bytes() == 64, "two live resources");
    registry.remove(a);
    const auto rows = registry.snapshot();
    check(rows.size() == 1 && rows[0].id == b && rows[0].label == "scene", "snapshot after removal");
    check(registry.report().find("1 live, 0 bytes") != std::string::npos, "report summary");
}

const char* shaderSource = R"(
@group(0) @binding(0) var<uniform> scale : vec4<f32>;
@vertex fn vs(@builtin(vertex_index) i : u32) -> @builtin(position) vec4<f32> {
    let p = array<vec2<f32>, 3>(vec2(0.0, 0.5), vec2(-0.5, -0.5), vec2(0.5, -0.5));
    return vec4<f32>(p[i] * scale.x, 0.0, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return vec4<f32>(1.0); }
)";

struct PosX { using type = float; };
struct PosY { using type = float; };
struct Kind { using type = uint8_t; };

template<class F>
bool throws(F&& f) {
    try { f(); } catch (const Lattice::ExceptionBase&) { return true; }
    return false;
}

void gpuSoAMirrorsCpuTable(WGPU::Device& device) {
    StdData::SoA atoms;
    atoms.addCol<PosX>();
    atoms.addCol<Kind>();
    atoms.addCol<PosY>();
    atoms.resize(3);
    check(atoms.bytes().size() % 4 == 0, "SoA block is whole words");

    WGPU::GpuSoA table{device, "atoms"};
    check(table.wanted(), "an empty table wants the first upload");
    table.upload(atoms);
    const auto first = table.generation();
    check(table.rows() == 3 && table.columns() == 3, "schema follows the source");
    check(table.column(1).type().is<uint8_t>() && table.column(2).size() == 3, "column views");
    check(table.offset(atoms.column(2).name()) == atoms.columnOffset(2), "same layout as the source");
    check(!table.wanted(), "upload is pending until read");

    bool seen = false;
    table.read([&](const WGPU::GpuSoA::View& view) {
        seen = view.rows == 3 && view.generation == first && view.buffer.size() >= atoms.bytes().size();
    });
    check(seen && table.wanted(), "read takes the upload");

    atoms.at<PosX>(0) = 1.0f;
    table.upload(atoms);
    check(table.generation() == first, "same layout keeps the buffer");
    atoms.resize(100);
    table.upload(atoms);
    check(table.generation() != first && table.rows() == 100, "growth recreates the buffer");

    check(throws([&] { (void)table.column(0).values<float>(); }), "no host access to GPU columns");
    check(throws([&] { table.addRow(1.0f, uint8_t{1}, 2.0f); }), "no host writes");
    check(throws([&] { (void)table.offset("missing"); }), "unknown column");

    // GPU-owned table: columns and growth on the device, rows are kept by a GPU copy.
    WGPU::GpuSoA owned{device, "owned"};
    owned.addColumn("x", Lattice::tableType<float>());
    owned.resize(10);
    const auto before = owned.generation();
    owned.addColumn("kind", Lattice::tableType<uint32_t>());
    owned.resize(1000);
    check(owned.rows() == 1000 && owned.columns() == 2 && owned.generation() != before, "GPU-owned growth");
    check(owned.offset("kind") % 4 == 0, "GPU-owned offsets are word aligned");

    // Writer thread uploads while this thread submits frames.
    std::thread writer([&] {
        for (int i = 0; i < 200; ++i) {
            atoms.at<PosY>(0) = float(i);
            table.upload(atoms);
        }
    });
    for (int i = 0; i < 200; ++i) {
        table.read([](const WGPU::GpuSoA::View&) {});
        WGPU::CommandList commands{device};
        commands.submit();
    }
    writer.join();
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

void explicitLayoutsAndDepth(WGPU::Device& device) {
    const auto errors = WGPU::errorCount();
    {
        const std::array<WGPU::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = WGPU::BindingType::Uniform}}};
        const std::array<WGPU::LayoutEntry, 1> dataEntries{{{.binding = 0,
            .type = WGPU::BindingType::ReadOnlyStorage, .visibility = WGPU::ShaderStage::Vertex}}};
        WGPU::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        WGPU::BindingLayout dataLayout{device, dataEntries, "data"};

        WGPU::Shader shader{device, {.source = layoutShader, .label = "layouts"}};
        // Two pipelines on the same camera layout: one bind group serves both.
        WGPU::Pipeline flat{device, shader, {.layouts = {&cameraLayout, &dataLayout}, .label = "flat"}};
        WGPU::Pipeline deep{device, shader, {.depthFormat = WGPU::TextureFormat::Depth32Float,
            .layouts = {&cameraLayout, &dataLayout}, .label = "deep"}};

        WGPU::Buffer camera{device, {.size = 64,
            .usage = WGPU::BufferUsage::Uniform | WGPU::BufferUsage::CopyDestination, .label = "camera"}};
        WGPU::Buffer data{device, {.size = 16,
            .usage = WGPU::BufferUsage::Storage | WGPU::BufferUsage::CopyDestination, .label = "data"}};
        const WGPU::Binding cameraBinding{.binding = 0, .buffer = &camera};
        const WGPU::Binding dataBinding{.binding = 0, .buffer = &data};
        WGPU::BindingSet cameraSet{device, cameraLayout, std::span(&cameraBinding, 1)};
        WGPU::BindingSet dataSet{device, dataLayout, std::span(&dataBinding, 1)};

        WGPU::Texture depth{device, {.width = 64, .height = 32, .format = WGPU::TextureFormat::Depth32Float,
            .usage = WGPU::TextureUsage::RenderAttachment, .label = "depth"}};
        check(depth.view() && depth.width() == 64 && depth.format() == WGPU::TextureFormat::Depth32Float,
            "depth texture has a view");
        WGPU::CommandList commands{device};
        commands.submit();
    }
    check(WGPU::errorCount() == errors, "explicit layouts, depth pipeline and bind groups are valid");
    check(device.resources().size() == 0, "layouts and textures are released");

    {
        const std::array<WGPU::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = WGPU::BindingType::Uniform}}};
        WGPU::BindingLayout cameraLayout{device, cameraEntries, "camera"};
        WGPU::Shader shader{device, {.source = layoutShader, .label = "layouts"}};
        try {
            WGPU::Pipeline missing{device, shader, {.layouts = {&cameraLayout}, .label = "missing group 1"}};
        } catch (const Lattice::ExceptionBase&) {}
    }
    check(WGPU::errorCount() > errors, "a pipeline without the data group is reported");
}

bool resourcesRegisterOnDevice() {
    WGPU::Instance instance;
    WGPU::Device* device = nullptr;
    try {
        device = new WGPU::Device(instance);
    } catch (const std::exception& error) {
        std::cout << "WGPU: no adapter, device checks skipped (" << error.what() << ")\n";
        return false;
    }
    {
        WGPU::Buffer uniform{*device, {.size = 16,
            .usage = WGPU::BufferUsage::Uniform | WGPU::BufferUsage::CopyDestination, .label = "scale"}};
        WGPU::Shader shader{*device, {.source = shaderSource, .label = "triangle"}};
        WGPU::Pipeline pipeline{*device, shader, {.label = "triangle"}};
        const WGPU::Binding binding{.binding = 0, .buffer = &uniform};
        WGPU::BindingSet bindings{*device, pipeline, 0, std::span(&binding, 1)};
        check(device->resources().size() == 4 && device->resources().bytes() == 16, "resources are tracked");
        const float scale[4] = {1, 1, 1, 1};
        device->writeBuffer(uniform, 0, std::as_bytes(std::span(scale)));
        WGPU::CommandList commands{*device};
        commands.submit();
    }
    check(device->resources().size() == 0, "RAII resources unregister");
    explicitLayoutsAndDepth(*device);
    gpuSoAMirrorsCpuTable(*device);
    check(device->resources().size() == 0, "GpuSoA releases its buffers");
    delete device;
    return true;
}
}

int main() {
    registryTracksLiveResources();
    const bool device = resourcesRegisterOnDevice();
    std::cout << "WGPU: registry" << (device ? ", device resources, submit, layouts, depth and GpuSoA" : "") << " passed\n";
}
