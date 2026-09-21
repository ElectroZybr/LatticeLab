#pragma once

#include "Surface.hpp"
#include "WGPUBuffer.hpp"
#include <GPU/include/CommandList.hpp>
#include <Lattice/Kernel/Exception.hpp>

namespace WGPU {

class BindingSet final : public GPU::BindingSet {
public:
    BindingSet(WGPUDevice device, Pipeline& pipeline, uint32_t group, std::span<const GPU::Binding> bindings)
        : device_(device)
    {
        auto layout = wgpuRenderPipelineGetBindGroupLayout(pipeline.native(), group);
        if (!layout)
            throw Lattice::Exception("WGPU::BindingSet", "failed to get bind group layout");

        std::vector<WGPUBindGroupEntry> entries;
        entries.reserve(bindings.size());

        for (const auto& binding : bindings) {
            auto* buffer = dynamic_cast<Buffer*>(binding.buffer);
            if (!buffer) {
                wgpuBindGroupLayoutRelease(layout);
                throw Lattice::Exception("WGPU::BindingSet", "expected WGPU buffer at binding {}", binding.binding);
            }

            WGPUBindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.buffer = buffer->native();
            entry.offset = binding.offset;
            entry.size = binding.size ? binding.size : buffer->size();

            entries.push_back(entry);
        }

        WGPUBindGroupDescriptor desc{};
        desc.layout = layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();

        bindGroup_ = wgpuDeviceCreateBindGroup(device_, &desc);

        wgpuBindGroupLayoutRelease(layout);

        if (!bindGroup_)
            throw Lattice::Exception("WGPU::BindingSet", "failed to create bind group");
    }

    ~BindingSet() override {
        if (bindGroup_)
            wgpuBindGroupRelease(bindGroup_);
    }

    WGPUBindGroup native() const noexcept {
        return bindGroup_;
    }

private:
    WGPUDevice device_ = nullptr;
    WGPUBindGroup bindGroup_ = nullptr;
};

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
        auto* native = dynamic_cast<Pipeline*>(&pipeline);
        if (!native || native->device() != device_)
            throw Lattice::Exception("WGPU::RenderPass", "expected pipeline from the same WGPU device");
        pipeline_ = native->native();
        wgpuRenderPassEncoderSetPipeline(pass_, pipeline_);
    }

    void setBindings(uint32_t group, GPU::BindingSet& bindings) override {
        requireActive();
        auto* native = dynamic_cast<BindingSet*>(&bindings);
        if (!native)
            throw Lattice::Exception("WGPU::RenderPass", "expected WGPU binding set");
        wgpuRenderPassEncoderSetBindGroup(pass_, group, native->native(), 0, nullptr);
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
