#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <Lattice/Kernel/Node.hpp>
#include "Buffer.hpp"
#include <GPU/include/CommandList.hpp>
#include <GPU/include/Device.hpp>
#include "GPUAPI.hpp"
#include "Shader.hpp"
#include "Texture.hpp"


namespace WGPU {

class Device;

static WGPUStringView toWGPU(std::string_view s) {
    return WGPUStringView{s.data(), static_cast<size_t>(s.size())};
}

class WGPU final : public GPU::GPUAPI {
    static constexpr std::string_view tag = "WGPU";
public:
    explicit WGPU(Lattice::Node&, const Desc& = {}) { createInstance(); }

    void configure(Lattice::Node& node) {
        node.add<Device>(deviceName());
    }

    ~WGPU() override {
        if (instance_)
            wgpuInstanceRelease(instance_);
    }

    WGPUDevice createDevice(WGPUAdapter& selectedAdapter);
    WGPUInstance native() const noexcept { return instance_; }
private:
    std::string deviceName();
    void createInstance();
    std::vector<WGPUAdapter> enumerateAdapters();
    WGPUAdapter selectAdapter(std::span<WGPUAdapter> adapters);

    WGPUInstance instance_ = nullptr;
};

class CommandList final : public GPU::CommandList {
public:
    CommandList(WGPUDevice device, WGPUQueue queue) : device_(device), queue_(queue) {
        wgpuQueueAddRef(queue_);
        WGPUCommandEncoderDescriptor desc{};
        encoder_ = wgpuDeviceCreateCommandEncoder(device, &desc);
        if (!encoder_) {
            wgpuQueueRelease(queue_);
            throw Lattice::Exception("WGPU::CommandList", "failed to create encoder");
        }
    }

    ~CommandList() override {
        if (commandBuffer_) wgpuCommandBufferRelease(commandBuffer_);
        if (encoder_) wgpuCommandEncoderRelease(encoder_);
        if (queue_) wgpuQueueRelease(queue_);
    }

    void draw(GPU::Surface&, GPU::Pipeline&, GPU::ClearColor, uint32_t) override;

    void submit() override {
        if (submitted_)
            throw Lattice::Exception("WGPU::CommandList", "command list already submitted");
        if (!commandBuffer_)
            commandBuffer_ = wgpuCommandEncoderFinish(encoder_, nullptr);

        if (!commandBuffer_)
            throw Lattice::Exception("WGPU::CommandList", "failed to finish encoder");
        wgpuQueueSubmit(queue_, 1, &commandBuffer_);
        submitted_ = true;
    }

private:
    WGPUDevice device_ = nullptr;
    bool submitted_ = false;
    WGPUQueue queue_ = nullptr;
    WGPUCommandEncoder encoder_ = nullptr;
    WGPUCommandBuffer commandBuffer_ = nullptr;
};

class Device final : public GPU::Device {
public:
    explicit Device(Lattice::Node& node, const Desc& = {}) {
        auto backend = node.requireParent<WGPU>();
        device_ = backend->createDevice(adapter_);
        instance_ = backend->native();
        wgpuInstanceAddRef(instance_);
        queue_ = wgpuDeviceGetQueue(device_);
    }
    
    ~Device() override {
        if (queue_)
            wgpuQueueRelease(queue_);
        if (device_)
            wgpuDeviceRelease(device_);
        if (adapter_) wgpuAdapterRelease(adapter_);
        if (instance_) wgpuInstanceRelease(instance_);
    }

    std::unique_ptr<GPU::CommandList> createCommandList() override {
        return std::make_unique<CommandList>(device_, queue_);
    }

    WGPUDevice native() const noexcept { return device_; }
    WGPUAdapter adapter() const noexcept { return adapter_; }
    WGPUInstance instance() const noexcept { return instance_; }

private:
    WGPUAdapter adapter_ = nullptr;
    WGPUInstance instance_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
};


// ---------- resources ----------
class Buffer final : public GPU::Buffer {
public:
    explicit Buffer(Lattice::Node& node, const Desc& desc) {
        const auto device = node.requireParent<Device>();

        WGPUBufferDescriptor nativeDesc = {};
        auto name = node.name();
        nativeDesc.label = WGPUStringView{name.data(), name.size()};
        nativeDesc.size = desc.size;
        nativeDesc.usage = static_cast<WGPUBufferUsage>(desc.usage);
        nativeDesc.mappedAtCreation = false;

        buffer_ = wgpuDeviceCreateBuffer(device->native(), &nativeDesc);

        if (!buffer_)
            throw Lattice::Exception("WGPU::Buffer", "failed to create buffer");
    }

    ~Buffer() override {
        if (buffer_)
            wgpuBufferRelease(buffer_);
    }

    WGPUBuffer native() const noexcept {
        return buffer_;
    }

private:
    WGPUBuffer buffer_ = nullptr;
};

class Texture final : public GPU::Texture {
public:
    explicit Texture(Lattice::Node& node, const Desc& desc) {
        auto device = node.requireParent<Device>();

        WGPUTextureDescriptor nativeDesc = {};
        nativeDesc.size = {desc.width, desc.height, desc.depth};
        nativeDesc.mipLevelCount = desc.mipLevels;
        nativeDesc.sampleCount = 1;
        nativeDesc.dimension = WGPUTextureDimension_2D;
        nativeDesc.format = toWGPU(desc.format);
        nativeDesc.usage = static_cast<WGPUTextureUsage>(desc.usage);

        texture_ = wgpuDeviceCreateTexture(device->native(), &nativeDesc);

        if (!texture_)
            throw Lattice::Exception("WGPU::Texture", "failed to create texture");
    }

    ~Texture() override {
        if (texture_)
            wgpuTextureRelease(texture_);
    }

    WGPUTexture native() const noexcept { return texture_; }

    constexpr WGPUTextureFormat toWGPU(GPU::TextureFormat format) {
        switch (format) {
            case GPU::TextureFormat::R8Unorm: return WGPUTextureFormat_R8Unorm;
            case GPU::TextureFormat::RG8Unorm: return WGPUTextureFormat_RG8Unorm;
            case GPU::TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
            case GPU::TextureFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
            case GPU::TextureFormat::R16Float: return WGPUTextureFormat_R16Float;
            case GPU::TextureFormat::RG16Float: return WGPUTextureFormat_RG16Float;
            case GPU::TextureFormat::RGBA16Float: return WGPUTextureFormat_RGBA16Float;
            case GPU::TextureFormat::R32Float: return WGPUTextureFormat_R32Float;
            case GPU::TextureFormat::RG32Float: return WGPUTextureFormat_RG32Float;
            case GPU::TextureFormat::RGBA32Float: return WGPUTextureFormat_RGBA32Float;
            case GPU::TextureFormat::Depth16Unorm: return WGPUTextureFormat_Depth16Unorm;
            case GPU::TextureFormat::Depth24Plus: return WGPUTextureFormat_Depth24Plus;
            case GPU::TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
            case GPU::TextureFormat::Depth32Float: return WGPUTextureFormat_Depth32Float;
            default: return WGPUTextureFormat_Undefined;
        }
    }

private:
    WGPUTexture texture_ = nullptr;
};

class Shader final : public GPU::Shader {
public:
    explicit Shader(Lattice::Node& node, const Desc& desc) {
        const auto device = node.requireParent<Device>();
        device_ = device->native();

        if (desc.language != GPU::ShaderLanguage::WGSL)
            throw Lattice::Exception("WGPU::Shader", "unsupported shader language");

        WGPUShaderSourceWGSL source{};
        source.chain.sType = WGPUSType_ShaderSourceWGSL;
        source.code = WGPUStringView{desc.source.data(), desc.source.size()};

        WGPUShaderModuleDescriptor nativeDesc{};
        nativeDesc.nextInChain = &source.chain;

        const auto name = node.name();
        nativeDesc.label = WGPUStringView{name.data(), name.size()};

        shader_ = wgpuDeviceCreateShaderModule(device->native(), &nativeDesc);

        if (!shader_)
            throw Lattice::Exception("WGPU::Shader", "failed to create shader");
    }

    ~Shader() override {
        if (shader_)
            wgpuShaderModuleRelease(shader_);
    }

    WGPUShaderModule native() const noexcept { return shader_; }
    WGPUDevice device() const noexcept { return device_; }

private:
    WGPUDevice device_ = nullptr;
    WGPUShaderModule shader_ = nullptr;
};

}
