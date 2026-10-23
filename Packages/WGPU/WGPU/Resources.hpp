#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <webgpu/webgpu.h>

#include "Device.hpp"
#include "Types.hpp"

namespace WGPU {

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

private:
    WGPUTexture texture_ = nullptr;
};

class Shader final : Tracked {
public:
    Shader(Device& device, const ShaderDesc& desc);
    ~Shader();

    WGPUShaderModule native() const noexcept { return shader_; }

private:
    WGPUShaderModule shader_ = nullptr;
};

class Pipeline final : Tracked {
public:
    Pipeline(Device& device, Shader& shader, const PipelineDesc& desc);
    ~Pipeline();

    WGPURenderPipeline native() const noexcept { return pipeline_; }
    Device& device() const noexcept { return Tracked::device(); }

private:
    WGPURenderPipeline pipeline_ = nullptr;
};

class BindingSet final : Tracked {
public:
    BindingSet(Device& device, Pipeline& pipeline, uint32_t group, std::span<const Binding> bindings);
    ~BindingSet();

    WGPUBindGroup native() const noexcept { return bindGroup_; }

private:
    WGPUBindGroup bindGroup_ = nullptr;
};

/// Presentation target of one native window.
class Surface final : Tracked {
public:
    Surface(Device& device, const NativeWindow& window);
    ~Surface();

    TextureFormat format() const;
    void attach(const NativeWindow& window);
    void resize(uint32_t width, uint32_t height);
    bool acquire();
    void present();
    void releaseFrame();

    WGPUTextureView view() const noexcept;
    Device& device() const noexcept { return Tracked::device(); }

private:
    void releaseSurface();

    struct State;
    std::unique_ptr<State> state_;
};

}
