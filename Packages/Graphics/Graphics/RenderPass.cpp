#include "RenderPass.hpp"

#include <Lattice/Tools/Exception.hpp>

using Error = Lattice::Exception<RenderPass>;

RenderPass::RenderPass(Wgpu::CommandList& commands, Surface& surface, Color clear, Wgpu::Texture* depth,
                       const WGPURenderPassTimestampWrites* timestamps)
    : commands_(commands) {
    if (&surface.device() != &commands.device() || !surface.view())
        throw Error("expected an acquired surface of the command list's device");
    begin(surface.view(), clear, depth, timestamps);
}

RenderPass::RenderPass(Wgpu::CommandList& commands, Wgpu::Texture& target, Color clear, Wgpu::Texture* depth,
                       const WGPURenderPassTimestampWrites* timestamps)
    : commands_(commands) {
    begin(target.view(), clear, depth, timestamps);
}

void RenderPass::begin(WGPUTextureView target, Color clear, Wgpu::Texture* depth,
                       const WGPURenderPassTimestampWrites* timestamps) {
    WGPURenderPassColorAttachment color{};
    color.view = target;
    color.loadOp = WGPULoadOp_Clear;
    color.storeOp = WGPUStoreOp_Store;
    color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color.clearValue = {clear.r, clear.g, clear.b, clear.a};

    WGPURenderPassDescriptor desc{};
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &color;
    desc.timestampWrites = timestamps;

    WGPURenderPassDepthStencilAttachment depthAttachment{};
    if (depth) {
        depthAttachment.view = depth->view();
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Discard;
        depthAttachment.depthClearValue = 1.0f;
        desc.depthStencilAttachment = &depthAttachment;
    }

    pass_ = wgpuCommandEncoderBeginRenderPass(commands_.encoder(), &desc);
    if (!pass_)
        throw Error("failed to begin render pass");
    commands_.passBegan();
}

RenderPass::~RenderPass() {
    end();
}

void RenderPass::requireActive() const {
    if (!pass_)
        throw Error("no active render pass");
}

void RenderPass::setViewport(Rect rect) {
    requireActive();
    wgpuRenderPassEncoderSetViewport(pass_, float(rect.x), float(rect.y), float(rect.width), float(rect.height),
        0.0f, 1.0f);
}

void RenderPass::setScissor(Rect rect) {
    requireActive();
    wgpuRenderPassEncoderSetScissorRect(pass_, rect.x, rect.y, rect.width, rect.height);
}

void RenderPass::setPipeline(RenderPipeline& pipeline) {
    requireActive();
    if (&pipeline.device() != &commands_.device())
        throw Error("expected a pipeline of the same device");
    wgpuRenderPassEncoderSetPipeline(pass_, pipeline.native());
    pipeline_ = true;
}

void RenderPass::setBindings(uint32_t group, Wgpu::BindingSet& bindings) {
    requireActive();
    wgpuRenderPassEncoderSetBindGroup(pass_, group, bindings.native(), 0, nullptr);
}

void RenderPass::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) {
    requireActive();
    if (!pipeline_)
        throw Error("draw requires a pipeline");
    wgpuRenderPassEncoderDraw(pass_, vertexCount, instanceCount, firstVertex, 0);
}

void RenderPass::end() {
    if (!pass_)
        return;
    wgpuRenderPassEncoderEnd(pass_);
    wgpuRenderPassEncoderRelease(pass_);
    pass_ = nullptr;
    commands_.passEnded();
}
