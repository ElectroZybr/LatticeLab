#include "CommandList.hpp"

#include <Lattice/Tools/Exception.hpp>

namespace WGPU {

RenderPass::~RenderPass() {
    end();
}

void RenderPass::begin(WGPUCommandEncoder encoder, Surface& surface, Color clear) {
    if (active())
        throw Lattice::Exception<RenderPass>("render pass already active");

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
        throw Lattice::Exception<RenderPass>("failed to begin render pass");

    device_ = &surface.device();
    pipeline_ = false;
}

void RenderPass::setViewport(Rect rect) {
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

void RenderPass::setScissor(Rect rect) {
    requireActive();
    wgpuRenderPassEncoderSetScissorRect(
        pass_,
        rect.x,
        rect.y,
        rect.width,
        rect.height
    );
}

void RenderPass::setPipeline(Pipeline& pipeline) {
    requireActive();
    if (&pipeline.device() != device_)
        throw Lattice::Exception<RenderPass>("expected pipeline from the same WGPU device");

    wgpuRenderPassEncoderSetPipeline(pass_, pipeline.native());
    pipeline_ = true;
}

void RenderPass::setBindings(uint32_t group, BindingSet& bindings) {
    requireActive();
    wgpuRenderPassEncoderSetBindGroup(pass_, group, bindings.native(), 0, nullptr);
}

void RenderPass::draw(uint32_t vertexCount, uint32_t firstVertex) {
    requireActive();
    if (!pipeline_)
        throw Lattice::Exception<RenderPass>("draw requires a pipeline");

    wgpuRenderPassEncoderDraw(pass_, vertexCount, 1, firstVertex, 0);
}

void RenderPass::end() {
    if (!pass_)
        return;

    wgpuRenderPassEncoderEnd(pass_);
    wgpuRenderPassEncoderRelease(pass_);
    pass_ = nullptr;
    pipeline_ = false;
    device_ = nullptr;
}

void RenderPass::requireActive() const {
    if (!pass_)
        throw Lattice::Exception<RenderPass>("no active render pass");
}

CommandList::CommandList(Device& device) : device_(device) {
    WGPUCommandEncoderDescriptor desc{};
    encoder_ = wgpuDeviceCreateCommandEncoder(device_.native(), &desc);
    if (!encoder_)
        throw Lattice::Exception<CommandList>("failed to create encoder");
}

CommandList::~CommandList() {
    renderPass_.end();

    if (commandBuffer_)
        wgpuCommandBufferRelease(commandBuffer_);
    if (encoder_)
        wgpuCommandEncoderRelease(encoder_);
}

RenderPass& CommandList::beginRenderPass(Surface& surface, Color clear) {
    if (submitted_ || commandBuffer_)
        throw Lattice::Exception<CommandList>("command list already finished");
    if (&surface.device() != &device_ || !surface.view())
        throw Lattice::Exception<CommandList>("expected acquired surface from the same WGPU device");

    renderPass_.begin(encoder_, surface, clear);
    return renderPass_;
}

void CommandList::submit() {
    if (renderPass_.active())
        throw Lattice::Exception<CommandList>("end the render pass before submitting");

    if (submitted_)
        throw Lattice::Exception<CommandList>("command list already submitted");

    if (!commandBuffer_)
        commandBuffer_ = wgpuCommandEncoderFinish(encoder_, nullptr);

    if (!commandBuffer_)
        throw Lattice::Exception<CommandList>("failed to finish encoder");

    wgpuQueueSubmit(device_.queue(), 1, &commandBuffer_);
    submitted_ = true;
}

}
