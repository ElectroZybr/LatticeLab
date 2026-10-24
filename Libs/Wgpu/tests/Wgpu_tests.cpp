// Registry logic always; device and resources only when an adapter is available.
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/GpuSoA.hpp>
#include <Wgpu/Resources.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void registryTracksLiveResources() {
    Wgpu::ResourceRegistry registry;
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

void gpuSoAMirrorsCpuTable(Wgpu::Device& device) {
    StdData::SoA atoms;
    atoms.addCol<PosX>();
    atoms.addCol<Kind>();
    atoms.addCol<PosY>();
    atoms.resize(3);
    check(atoms.bytes().size() % 4 == 0, "SoA block is whole words");

    Wgpu::GpuSoA table{device, "atoms"};
    check(table.wanted(), "an empty table wants the first upload");
    table.upload(atoms);
    const auto first = table.generation();
    check(table.rows() == 3 && table.columns() == 3, "schema follows the source");
    check(table.column(1).type().is<uint8_t>() && table.column(2).size() == 3, "column views");
    check(table.offset(atoms.column(2).name()) == atoms.columnOffset(2), "same layout as the source");
    check(!table.wanted(), "upload is pending until read");

    bool seen = false;
    table.read([&](const Wgpu::GpuSoA::View& view) {
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
    Wgpu::GpuSoA owned{device, "owned"};
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
        table.read([](const Wgpu::GpuSoA::View&) {});
        Wgpu::CommandList commands{device};
        commands.submit();
    }
    writer.join();
}

void layoutsAndTexturesOnDevice(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        const std::array<Wgpu::LayoutEntry, 2> entries{{
            {.binding = 0, .type = Wgpu::BindingType::Uniform},
            {.binding = 1, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex}}};
        Wgpu::BindingLayout layout{device, entries, "camera+data"};
        Wgpu::Buffer camera{device, {.size = 64,
            .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "camera"}};
        Wgpu::Buffer data{device, {.size = 16,
            .usage = Wgpu::BufferUsage::Storage | Wgpu::BufferUsage::CopyDestination, .label = "data"}};
        const std::array bindings{Wgpu::Binding{.binding = 0, .buffer = &camera}, Wgpu::Binding{.binding = 1, .buffer = &data}};
        Wgpu::BindingSet set{device, layout, bindings};

        Wgpu::Texture depth{device, {.width = 64, .height = 32, .format = Wgpu::TextureFormat::Depth32Float,
            .usage = Wgpu::TextureUsage::RenderAttachment, .label = "depth"}};
        check(depth.view() && depth.width() == 64 && depth.format() == Wgpu::TextureFormat::Depth32Float,
            "texture has a view");
        Wgpu::CommandList commands{device};
        commands.submit();
    }
    check(Wgpu::errorCount() == errors, "explicit layouts, bind groups and textures are valid");
    check(device.resources().size() == 0, "layouts and textures are released");
}

void mirrorCopiesOnTheOwnerThread(Wgpu::Device& device) {
    StdData::SoA atoms;
    atoms.addCol<PosX>();
    atoms.resize(2);
    Tasks::Queue owner;
    {
        Wgpu::GpuSoA table{device, {.table = atoms, .owner = owner}, "mirrored"};
        check(table.rows() == 2 && table.idle(), "the first copy is made at once without a running owner");

        std::atomic<bool> stop{false};
        std::thread thread([&] {
            owner.beginServing();
            while (!stop)
                owner.runUntil(std::chrono::steady_clock::now() + std::chrono::milliseconds(2),
                    [&] { return stop.load(); });
            owner.endServing();
        });
        while (!owner.serving())
            std::this_thread::yield();
        // The owner changes its data; the reader takes a copy and asks for the next one.
        owner.call([&] { atoms.resize(5); });
        table.read([](const Wgpu::GpuSoA::View&) {});
        while (!table.idle())
            std::this_thread::yield();
        check(table.rows() == 5, "the next copy is made by a command of the owner");

        table.detach();
        table.read([](const Wgpu::GpuSoA::View&) {});
        check(table.idle(), "a detached mirror requests no copies");
        stop = true;
        owner.wake();
        thread.join();
    }
}

bool resourcesRegisterOnDevice() {
    Wgpu::Instance instance;
    Wgpu::Device* device = nullptr;
    try {
        device = new Wgpu::Device(instance);
    } catch (const std::exception& error) {
        std::cout << "Wgpu: no adapter, device checks skipped (" << error.what() << ")\n";
        return false;
    }
    {
        Wgpu::Buffer uniform{*device, {.size = 16,
            .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "scale"}};
        Wgpu::Shader shader{*device, {.source = shaderSource, .label = "triangle"}};
        const std::array<Wgpu::LayoutEntry, 1> entries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
        Wgpu::BindingLayout layout{*device, entries, "scale"};
        const Wgpu::Binding binding{.binding = 0, .buffer = &uniform};
        Wgpu::BindingSet bindings{*device, layout, std::span(&binding, 1)};
        check(device->resources().size() == 4 && device->resources().bytes() == 16, "resources are tracked");
        const float scale[4] = {1, 1, 1, 1};
        device->writeBuffer(uniform, 0, std::as_bytes(std::span(scale)));
        Wgpu::CommandList commands{*device};
        commands.submit();
    }
    check(device->resources().size() == 0, "RAII resources unregister");
    layoutsAndTexturesOnDevice(*device);
    gpuSoAMirrorsCpuTable(*device);
    mirrorCopiesOnTheOwnerThread(*device);
    check(device->resources().size() == 0, "GpuSoA releases its buffers");
    delete device;
    return true;
}
}

int main() {
    registryTracksLiveResources();
    const bool device = resourcesRegisterOnDevice();
    std::cout << "Wgpu: registry" << (device ? ", device resources, layouts, textures, submit and GpuSoA (mirror)" : "") << " passed\n";
}
