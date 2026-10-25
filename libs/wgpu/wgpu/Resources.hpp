#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <webgpu/webgpu.h>

#include "Device.hpp"
#include "Types.hpp"

namespace Wgpu {

class Buffer final : Tracked {
public:
    Buffer(Device& device, const BufferDesc& desc);
    ~Buffer();

    WGPUBuffer native() const noexcept { return buffer_; }
    uint64_t size() const noexcept { return size_; }

private:
    WGPUBuffer buffer_ = nullptr;
    uint64_t size_ = 0;
};

class Texture final : Tracked {
public:
    Texture(Device& device, const TextureDesc& desc);
    ~Texture();

    WGPUTexture native() const noexcept { return texture_; }
    /// Default view of the whole texture (render attachments).
    WGPUTextureView view() const noexcept { return view_; }
    uint32_t width() const noexcept { return width_; }
    uint32_t height() const noexcept { return height_; }
    TextureFormat format() const noexcept { return format_; }

private:
    WGPUTexture texture_ = nullptr;
    WGPUTextureView view_ = nullptr;
    uint32_t width_ = 0, height_ = 0;
    TextureFormat format_ = TextureFormat::Undefined;
};

class Shader final : Tracked {
public:
    Shader(Device& device, const ShaderDesc& desc);
    ~Shader();

    WGPUShaderModule native() const noexcept { return shader_; }

private:
    WGPUShaderModule shader_ = nullptr;
};

/// A compute shader entry point on explicit bind group layouts.
class ComputePipeline final : Tracked {
public:
    ComputePipeline(Device& device, Shader& shader, const ComputePipelineDesc& desc);
    ~ComputePipeline();

    WGPUComputePipeline native() const noexcept { return pipeline_; }
    Device& device() const noexcept { return Tracked::device(); }

private:
    WGPUComputePipeline pipeline_ = nullptr;
};

/// Explicit bind group layout: shareable between pipelines and bind groups.
class BindingLayout final : Tracked {
public:
    BindingLayout(Device& device, std::span<const LayoutEntry> entries, std::string label = {});
    ~BindingLayout();

    WGPUBindGroupLayout native() const noexcept { return layout_; }

private:
    WGPUBindGroupLayout layout_ = nullptr;
};

class BindingSet final : Tracked {
public:
    /// Bind group for an explicit layout: usable with every pipeline built on it.
    BindingSet(Device& device, BindingLayout& layout, std::span<const Binding> bindings);
    ~BindingSet();

    WGPUBindGroup native() const noexcept { return bindGroup_; }

private:
    void create(WGPUBindGroupLayout layout, std::span<const Binding> bindings);

    WGPUBindGroup bindGroup_ = nullptr;
};

}
