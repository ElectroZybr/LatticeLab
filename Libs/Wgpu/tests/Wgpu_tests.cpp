// Registry logic always; device and resources only when an adapter is available.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/ComputePass.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/GpuSoA.hpp>
#include <Wgpu/Resources.hpp>
#include <Wgpu/Timer.hpp>

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

constexpr unsigned char TriangleShader[] = {
#include "triangle.wgsl.h"
};

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

    atoms.at<PosX>(1) = 7.5f;
    table.upload(atoms);
    bool seen = false, same = false;
    table.read([&](const Wgpu::GpuSoA::View& view) {
        seen = view.rows == 3 && view.generation == first && view.buffer.size() >= atoms.bytes().size();
        const auto copy = device.readBuffer(view.buffer, 0, atoms.bytes().size());
        same = std::equal(copy.begin(), copy.end(), atoms.bytes().begin());
    });
    check(same, "the GPU block is a byte copy of the CPU block");
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

constexpr unsigned char DoubleShader[] = {
#include "double.wgsl.h"
};

void computeDoublesAnArray(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        constexpr size_t count = 1000;   // not a multiple of the workgroup size
        std::vector<float> values(count);
        for (size_t i = 0; i < count; ++i)
            values[i] = float(i);
        Wgpu::Buffer data{device, {.size = count * sizeof(float), .usage = Wgpu::BufferUsage::Storage |
            Wgpu::BufferUsage::CopyDestination | Wgpu::BufferUsage::CopySource, .label = "data"}};
        device.writeBuffer(data, 0, std::as_bytes(std::span(values)));

        const std::array<Wgpu::LayoutEntry, 1> entries{{{.binding = 0, .type = Wgpu::BindingType::Storage,
            .visibility = Wgpu::ShaderStage::Compute}}};
        Wgpu::BindingLayout layout{device, entries, "data"};
        const Wgpu::Binding binding{.binding = 0, .buffer = &data};
        Wgpu::BindingSet set{device, layout, std::span(&binding, 1)};
        Wgpu::Shader shader{device, {.source = reinterpret_cast<const char*>(DoubleShader), .label = "double"}};
        Wgpu::ComputePipeline pipeline{device, shader, {.layouts = {&layout}, .label = "double"}};

        Wgpu::CommandList commands{device};
        {
            Wgpu::ComputePass pass{commands};
            pass.setPipeline(pipeline);
            pass.setBindings(0, set);
            pass.dispatch(Wgpu::workgroups(count, 64));
            check(throws([&] { commands.submit(); }), "submit waits for the open pass");
        }
        commands.submit();

        const auto bytes = device.readBuffer(data, 0, count * sizeof(float));
        const auto* doubled = reinterpret_cast<const float*>(bytes.data());
        bool all = true;
        for (size_t i = 0; i < count; ++i)
            all &= doubled[i] == 2.0f * float(i);
        check(all, "every element doubled, the tail of the last workgroup included");
        check(device.readBuffer(data, 4, 8).size() == 8, "partial read");
        check(throws([&] { device.readBuffer(data, 2, 4); }), "unaligned read is rejected");
    }
    check(Wgpu::errorCount() == errors, "compute pipeline, pass and readback are valid");
}

void timerMeasuresPasses(Wgpu::Device& device) {
    const auto errors = Wgpu::errorCount();
    {
        Wgpu::Timer timer{device, 2, "test"};
        if (!timer.available()) {
            std::cout << "Wgpu: " << timer.unavailableReason() << ", timer checks skipped\n";
            return;
        }
        constexpr size_t count = 1 << 21;   // 32768 groups of 64: under the 65535 limit
        Wgpu::Buffer data{device, {.size = count * sizeof(float), .usage = Wgpu::BufferUsage::Storage, .label = "data"}};
        const std::array<Wgpu::LayoutEntry, 1> entries{{{.binding = 0, .type = Wgpu::BindingType::Storage,
            .visibility = Wgpu::ShaderStage::Compute}}};
        Wgpu::BindingLayout layout{device, entries, "data"};
        const Wgpu::Binding binding{.binding = 0, .buffer = &data};
        Wgpu::BindingSet set{device, layout, std::span(&binding, 1)};
        Wgpu::Shader shader{device, {.source = reinterpret_cast<const char*>(DoubleShader), .label = "double"}};
        Wgpu::ComputePipeline pipeline{device, shader, {.layouts = {&layout}, .label = "double"}};

        // Three passes into a timer of two intervals: the third runs untimed.
        const auto started = std::chrono::steady_clock::now();
        Wgpu::CommandList commands{device};
        for (int i = 0; i < 3; ++i) {
            Wgpu::ComputePass pass{commands, "double", &timer};
            pass.setPipeline(pipeline);
            pass.setBindings(0, set);
            pass.dispatch(Wgpu::workgroups(count, 64));
        }
        const uint32_t copy = timer.begin(commands, "copy");
        timer.end(commands, copy);
        commands.submit();
        device.wait();
        const double wall = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - started).count();

        const auto intervals = timer.read();
        check(intervals.size() == 2 && intervals[0].name == "double", "intervals by name");
        check(intervals[0].count == 2 && intervals[0].skipped == 1, "a full timer skips and counts");
        check(intervals[0].nanoseconds > 0 && intervals[0].nanoseconds < wall, "GPU time within the wall");
        check(intervals[1].count == 0 && intervals[1].skipped == 1, "no room for the copy interval");
        check(timer.read()[0].count == 0, "read starts over");
    }
    check(Wgpu::errorCount() == errors, "timestamp queries are valid");
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
        Wgpu::Shader shader{*device, {.source = reinterpret_cast<const char*>(TriangleShader), .label = "triangle"}};
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
    computeDoublesAnArray(*device);
    timerMeasuresPasses(*device);
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
    std::cout << "Wgpu: registry" << (device ? ", device resources, layouts, textures, compute, timer, readback and GpuSoA (mirror)" : "") << " passed\n";
}
