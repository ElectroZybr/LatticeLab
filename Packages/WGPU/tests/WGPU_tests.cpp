// Registry logic always; device and resources only when an adapter is available.
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <WGPU/CommandList.hpp>
#include <WGPU/Device.hpp>
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
    delete device;
    return true;
}
}

int main() {
    registryTracksLiveResources();
    const bool device = resourcesRegisterOnDevice();
    std::cout << "WGPU: registry" << (device ? ", device resources and submit" : "") << " passed\n";
}
