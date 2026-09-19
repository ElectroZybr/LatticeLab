#pragma once

#include "Surface.hpp"
#include <GPU/include/CommandList.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <cstring>

namespace WGPU {

class RenderPass final : public GPU::RenderPass {
public:
    ~RenderPass() override { end(); }

    bool active() const noexcept { return pass_ != nullptr; }

    void begin(WGPUCommandEncoder encoder, Surface& surface, GPU::Color clear) {
        if (active()) throw Lattice::Exception("WGPU::RenderPass", "render pass already active");
        WGPURenderPassColorAttachment color{};
        color.view = surface.view();
        color.loadOp = WGPULoadOp_Clear;
        color.storeOp = WGPUStoreOp_Store;
        color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color.clearValue = {clear.r, clear.g, clear.b, clear.a};
        WGPURenderPassDescriptor desc{};
        desc.colorAttachmentCount = 1;
        desc.colorAttachments = &color;
        pass_ = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
        if (!pass_) throw Lattice::Exception("WGPU::RenderPass", "failed to begin render pass");
        device_ = surface.device();
        pipeline_ = nullptr;
    }

    void setViewport(GPU::Rect rect) override {
        requireActive();
        wgpuRenderPassEncoderSetViewport(pass_, float(rect.x), float(rect.y), float(rect.width), float(rect.height), 0.0f, 1.0f);
    }

    void setScissor(GPU::Rect rect) override {
        requireActive();
        wgpuRenderPassEncoderSetScissorRect(pass_, rect.x, rect.y, rect.width, rect.height);
    }

    void setPipeline(GPU::Pipeline& pipeline) override {
        requireActive();
        auto* native = dynamic_cast<WGPU::Pipeline*>(&pipeline);
        if (!native || native->device() != device_)
            throw Lattice::Exception("WGPU::RenderPass", "expected pipeline from the same WGPU device");
        pipeline_ = native->native();
        wgpuRenderPassEncoderSetPipeline(pass_, pipeline_);
    }

    void setUniform(uint32_t group, uint32_t binding, std::span<const std::byte> data) override {
        requireActive();
        if (!pipeline_ || data.empty())
            throw Lattice::Exception("WGPU::RenderPass", "uniform requires a pipeline and nonempty data");
        WGPUBufferDescriptor desc{};
        desc.size = (data.size() + 15) & ~uint64_t(15);
        desc.usage = WGPUBufferUsage_Uniform;
        desc.mappedAtCreation = true;
        auto buffer = wgpuDeviceCreateBuffer(device_, &desc);
        if (!buffer) throw Lattice::Exception("WGPU::RenderPass", "failed to create uniform buffer");
        auto* mapped = wgpuBufferGetMappedRange(buffer, 0, desc.size);
        if (!mapped) {
            wgpuBufferRelease(buffer);
            throw Lattice::Exception("WGPU::RenderPass", "failed to map uniform buffer");
        }
        std::memset(mapped, 0, desc.size);
        std::memcpy(mapped, data.data(), data.size());
        wgpuBufferUnmap(buffer);
        auto layout = wgpuRenderPipelineGetBindGroupLayout(pipeline_, group);
        WGPUBindGroupEntry entry{};
        entry.binding = binding;
        entry.buffer = buffer;
        entry.size = desc.size;
        WGPUBindGroupDescriptor bindDesc{};
        bindDesc.layout = layout;
        bindDesc.entryCount = 1;
        bindDesc.entries = &entry;
        auto bindGroup = wgpuDeviceCreateBindGroup(device_, &bindDesc);
        if (layout) wgpuBindGroupLayoutRelease(layout);
        wgpuBufferRelease(buffer);
        if (!bindGroup) throw Lattice::Exception("WGPU::RenderPass", "failed to create uniform bind group");
        // Encoded commands retain their own references; each draw gets immutable data.
        wgpuRenderPassEncoderSetBindGroup(pass_, group, bindGroup, 0, nullptr);
        wgpuBindGroupRelease(bindGroup);
    }

    void draw(uint32_t vertexCount, uint32_t firstVertex = 0) override {
        requireActive();
        if (!pipeline_) throw Lattice::Exception("WGPU::RenderPass", "draw requires a pipeline");
        wgpuRenderPassEncoderDraw(pass_, vertexCount, 1, firstVertex, 0);
    }

    void end() override {
        if (!pass_) return;
        wgpuRenderPassEncoderEnd(pass_);
        wgpuRenderPassEncoderRelease(pass_);
        pass_ = nullptr;
        pipeline_ = nullptr;
        device_ = nullptr;
    }

private:
    void requireActive() const {
        if (!pass_) throw Lattice::Exception("WGPU::RenderPass", "no active render pass");
    }
    WGPURenderPassEncoder pass_ = nullptr;
    WGPURenderPipeline pipeline_ = nullptr;
    WGPUDevice device_ = nullptr;
};

}
