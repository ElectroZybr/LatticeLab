#include "Render.hpp"

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include "Shell/include/WindowAPI.hpp"

// Plugin dependences
#include "WGPU.hpp"
#include "WindowAPI.hpp"


struct Render::FrameState {
    WGPUSurface surface = nullptr;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    WGPUTexture depth = nullptr;
    WGPUTextureView depthView = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    bool surfaceConfigured = false;
};

Render::Render(Lattice::Node& renderer)
    : frameState(std::make_unique<FrameState>()) {
    renderer.add<WGPU::WGPU>();
}

void Render::configure(Lattice::Node& renderer) {
    gpu_ = renderer.require<WGPU::WGPU>();
    window_ = renderer.find<WindowAPI>();
}

void Render::setup() {
    if (!window_ || !gpu_)
        return;

    ensureSurface(*window_);
    const auto fb = window_->framebufferSize();
    resize(uint32_t(fb.x), uint32_t(fb.y));
}

Render::~Render() {
    releaseFrameResources();
}

void Render::releaseFrameResources() {
    if (!frameState) return;

    if (frameState->depthView) {
        wgpuTextureViewRelease(frameState->depthView);
        frameState->depthView = nullptr;
    }
    if (frameState->depth) {
        wgpuTextureDestroy(frameState->depth);
        wgpuTextureRelease(frameState->depth);
        frameState->depth = nullptr;
    }
    if (frameState->surface) {
        if (frameState->surfaceConfigured) {
            wgpuSurfaceUnconfigure(frameState->surface);
            frameState->surfaceConfigured = false;
        }
        wgpuSurfaceRelease(frameState->surface);
        frameState->surface = nullptr;
    }

    frameState->width = 0;
    frameState->height = 0;
}

void Render::ensureSurface(WindowAPI& window) {
    if (frameState->surface) return;
    gpu_->init();
    const auto n = window.native();
    frameState->surface = gpu_->createSurface(window.native());
    if (!frameState->surface)
        throw Lattice::Exception(tag, "failed to create surface");
}

void Render::resize(uint32_t w, uint32_t h) {
    if (!frameState->surface || w == 0 || h == 0) return;
    if (w == frameState->width && h == frameState->height && frameState->surfaceConfigured)
        return;

    // depth
    if (frameState->depthView) {
        wgpuTextureViewRelease(frameState->depthView);
        frameState->depthView = nullptr;
    }
    if (frameState->depth) {
        wgpuTextureDestroy(frameState->depth);
        wgpuTextureRelease(frameState->depth);
        frameState->depth = nullptr;
    }

    frameState->format = gpu_->configureSurface(frameState->surface, w, h);
    frameState->surfaceConfigured = true;

    frameState->depth = gpu_->createDepthTexture(w, h);
    frameState->depthView = gpu_->createDepthTextureView(frameState->depth);

    frameState->width = w;
    frameState->height = h;
}

void Render::frame() {
    if (!window_)
        return;

    if (!frameState->surface || !frameState->surfaceConfigured) {
        setup();
        if (!frameState->surface || !frameState->surfaceConfigured)
            return;
    }

    WindowAPI& window = *window_;
    if (window.shouldClose()) return;
    const auto fb = window.framebufferSize();
    const uint32_t w = uint32_t(fb.x);
    const uint32_t h = uint32_t(fb.y);
    if (w == 0 || h == 0) return;

    resize(w, h);

    WGPUSurfaceTexture st{};
    wgpuSurfaceGetCurrentTexture(frameState->surface, &st);

    const bool ok =
        st.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal ||
        st.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal;

    if (!ok) {
        if (st.texture)
            wgpuTextureRelease(st.texture);

        if (st.status == WGPUSurfaceGetCurrentTextureStatus_Outdated ||
            st.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
            frameState->surfaceConfigured = false;
            resize(w, h);
        }
        return;
    }

    WGPUTextureView view = wgpuTextureCreateView(st.texture, nullptr);

    WGPUCommandEncoderDescriptor encDesc{};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpu_->device(), &encDesc);

    WGPURenderPassColorAttachment color{};
    color.view = view;
    color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color.loadOp = WGPULoadOp_Clear;
    color.storeOp = WGPUStoreOp_Store;
    color.clearValue = {0.1, 0.2, 0.3, 1.0};

    WGPURenderPassDepthStencilAttachment depthAtt{};
    depthAtt.view = frameState->depthView;
    depthAtt.depthLoadOp = WGPULoadOp_Clear;
    depthAtt.depthStoreOp = WGPUStoreOp_Store;
    depthAtt.depthClearValue = 1.0f;
    depthAtt.depthReadOnly = false;
    depthAtt.stencilLoadOp = WGPULoadOp_Undefined;
    depthAtt.stencilStoreOp = WGPUStoreOp_Undefined;
    depthAtt.stencilReadOnly = true;

    WGPURenderPassDescriptor passDesc{};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &color;
    passDesc.depthStencilAttachment = frameState->depthView ? &depthAtt : nullptr;


    std::ifstream file("Plugins/Render/src/shaders/circle.wgsl");
    std::string shaderSource(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    WGPUShaderSourceWGSL wgsl{};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = {shaderSource.data(), shaderSource.size()};

    WGPUShaderModuleDescriptor wgslDesc{};
    wgslDesc.label = {"CircleShader", 12};
    wgslDesc.nextInChain = &wgsl.chain;

    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu_->device(), &wgslDesc);

    WGPUVertexState vertex{};
    vertex.module = shader;
    vertex.entryPoint = {"vs", 2};

    WGPUColorTargetState colorTarget{};
    colorTarget.format = frameState->format;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragment{};
    fragment.module = shader;
    fragment.entryPoint = {"fs", 2};
    fragment.targetCount = 1;
    fragment.targets = &colorTarget;

    WGPUDepthStencilState depthStencil{};
    depthStencil.format = WGPUTextureFormat_Depth24Plus;
    depthStencil.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencil.depthCompare = WGPUCompareFunction_Less;

    
    WGPURenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = {"CirclePipeline", 14};
    pipelineDesc.vertex = vertex;
    pipelineDesc.fragment = &fragment;
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = 0xFFFFFFFF;
    pipelineDesc.depthStencil = &depthStencil;
    pipelineDesc.layout = nullptr;


    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(gpu_->device(), &pipelineDesc);

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);

    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    wgpuRenderPassEncoderDraw(pass, 6, 1, 0, 0);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(gpu_->queue(), 1, &cmd);
    wgpuCommandBufferRelease(cmd);

    wgpuSurfacePresent(frameState->surface);

    wgpuTextureViewRelease(view);
    wgpuTextureRelease(st.texture);
}