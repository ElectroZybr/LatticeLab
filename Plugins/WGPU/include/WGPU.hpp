#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <Lattice/Kernel/Node.hpp>
#include <GPU/include/CommandList.hpp>
#include <GPU/include/Device.hpp>
#include "GPUAPI.hpp"
#include "WGPUDevice.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Texture.hpp"


namespace WGPU {

static WGPUStringView toWGPU(std::string_view s) {
    return WGPUStringView{s.data(), static_cast<size_t>(s.size())};
}

class WGPU final : public GPU::GPUAPI {
    static constexpr std::string_view tag = "WGPU";
public:
    explicit WGPU(Lattice::Node&, const Desc& = {}) { createInstance(); }

    void configure(Lattice::Node& node) {
        const auto name = deviceName();
        node.add<Device>(name).focus<GPU::Device>();
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
        renderPass_.end();
        if (commandBuffer_) wgpuCommandBufferRelease(commandBuffer_);
        if (encoder_) wgpuCommandEncoderRelease(encoder_);
        if (queue_) wgpuQueueRelease(queue_);
    }

    GPU::RenderPass& beginRenderPass(GPU::Surface& surface, GPU::Color clear = {}) override {
        if (submitted_ || commandBuffer_)
            throw Lattice::Exception("WGPU::CommandList", "command list already finished");
        auto* native = dynamic_cast<::WGPU::Surface*>(&surface);
        if (!native || native->device() != device_ || !native->view())
            throw Lattice::Exception("WGPU::CommandList", "expected acquired surface from the same WGPU device");
        renderPass_.begin(encoder_, *native, clear);
        return renderPass_;
    }

    void submit() override {
        if (renderPass_.active())
            throw Lattice::Exception("WGPU::CommandList", "end the render pass before submitting");
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
    RenderPass renderPass_;
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
