#include <vector>

#include "WGPURenderPass.hpp"
#include "WGPUSurface.hpp"
#include "WGPUBuffer.hpp"

#include <Lattice/Kernel/Exception.hpp>

namespace WGPU {

BindingSet::BindingSet(
    WGPUDevice device,
    Pipeline& pipeline,
    uint32_t group,
    std::span<const GPU::Binding> bindings
) {
    auto layout = wgpuRenderPipelineGetBindGroupLayout(pipeline.native(), group);
    if (!layout)
        throw Lattice::Exception("WGPU::BindingSet", "failed to get bind group layout");

    std::vector<WGPUBindGroupEntry> entries;
    entries.reserve(bindings.size());

    for (const auto& binding : bindings) {
        auto* buffer = dynamic_cast<Buffer*>(binding.buffer);
        if (!buffer) {
            wgpuBindGroupLayoutRelease(layout);
            throw Lattice::Exception(
                "WGPU::BindingSet",
                "expected WGPU buffer at binding {}",
                binding.binding
            );
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

    bindGroup_ = wgpuDeviceCreateBindGroup(device, &desc);
    wgpuBindGroupLayoutRelease(layout);

    if (!bindGroup_)
        throw Lattice::Exception("WGPU::BindingSet", "failed to create bind group");
}

BindingSet::~BindingSet() {
    if (bindGroup_)
        wgpuBindGroupRelease(bindGroup_);
}

WGPUBindGroup BindingSet::native() const noexcept {
    return bindGroup_;
}

RenderPass::~RenderPass() {
    end();
}

bool RenderPass::active() const noexcept {
    return pass_ != nullptr;
}

void RenderPass::begin(WGPUCommandEncoder encoder, Surface& surface, GPU::Color clear) {
    if (active())
        throw Lattice::Exception("WGPU::RenderPass", "render pass already active");

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
    if (!pass_)
        throw Lattice::Exception("WGPU::RenderPass", "failed to begin render pass");

    device_ = surface.device();
    pipeline_ = nullptr;
}

void RenderPass::setViewport(GPU::Rect rect) {
    requireActive();
    wgpuRenderPassEncoderSetViewport(
        pass_,
        static_cast<float>(rect.x),
        static_cast<float>(rect.y),
        static_cast<float>(rect.width),
        static_cast<float>(rect.height),
        0.0f,
        1.0f
    );
}

void RenderPass::setScissor(GPU::Rect rect) {
    requireActive();
    wgpuRenderPassEncoderSetScissorRect(
        pass_,
        rect.x,
        rect.y,
        rect.width,
        rect.height
    );
}

void RenderPass::setPipeline(GPU::Pipeline& pipeline) {
    requireActive();

    auto* native = dynamic_cast<Pipeline*>(&pipeline);
    if (!native || native->device() != device_)
        throw Lattice::Exception(
            "WGPU::RenderPass",
            "expected pipeline from the same WGPU device"
        );

    pipeline_ = native->native();
    wgpuRenderPassEncoderSetPipeline(pass_, pipeline_);
}

void RenderPass::setBindings(uint32_t group, GPU::BindingSet& bindings) {
    requireActive();

    auto* native = dynamic_cast<BindingSet*>(&bindings);
    if (!native)
        throw Lattice::Exception("WGPU::RenderPass", "expected WGPU binding set");

    wgpuRenderPassEncoderSetBindGroup(pass_, group, native->native(), 0, nullptr);
}

void RenderPass::draw(uint32_t vertexCount, uint32_t firstVertex) {
    requireActive();
    if (!pipeline_)
        throw Lattice::Exception("WGPU::RenderPass", "draw requires a pipeline");

    wgpuRenderPassEncoderDraw(pass_, vertexCount, 1, firstVertex, 0);
}

void RenderPass::end() {
    if (!pass_)
        return;

    wgpuRenderPassEncoderEnd(pass_);
    wgpuRenderPassEncoderRelease(pass_);
    pass_ = nullptr;
    pipeline_ = nullptr;
    device_ = nullptr;
}

void RenderPass::requireActive() const {
    if (!pass_)
        throw Lattice::Exception("WGPU::RenderPass", "no active render pass");
}

}
