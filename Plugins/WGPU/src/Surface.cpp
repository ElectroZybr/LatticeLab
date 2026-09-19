#include "Surface.hpp"
#include "WGPU.hpp"
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#endif

namespace WGPU {

namespace {
WGPUTextureFormat nativeFormat(GPU::TextureFormat format) {
    switch (format) {
        case GPU::TextureFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case GPU::TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        default: throw Lattice::Exception("WGPU", "unsupported presentation format");
    }
}

WGPUSurface createSurface(WGPUInstance instance, const NativeWindow& window) {
    WGPUSurfaceDescriptor desc{};
    using Kind = NativeWindow::Kind;
    switch (window.kind) {
    #if defined(__linux__)
        case Kind::X11: {
            WGPUSurfaceSourceXlibWindow source{};
            source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
            source.display = window.display;
            source.window = reinterpret_cast<uintptr_t>(window.window);
            desc.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &desc);
        }
        case Kind::Wayland: {
            WGPUSurfaceSourceWaylandSurface source{};
            source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
            source.display = window.display;
            source.surface = window.window;
            desc.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &desc);
        }
    #elif defined(_WIN32)
        case Kind::Win32: {
            WGPUSurfaceSourceWindowsHWND source{};
            source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
            source.hinstance = window.extra ? window.extra : GetModuleHandleW(nullptr);
            source.hwnd = window.window;
            desc.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &desc);
        }
    #elif defined(__APPLE__)
        case Kind::Metal: {
            WGPUSurfaceSourceMetalLayer source{};
            source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
            source.layer = window.extra;
            desc.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &desc);
        }
    #endif
        default: throw Lattice::Exception("WGPU::Surface", "unsupported native window");
    }
}
}

struct Surface::State {
    std::shared_ptr<void> windowOwner;
    WGPUSurface surface = nullptr;
    WGPUDevice device = nullptr;
    WGPUTexture texture = nullptr;
    WGPUTextureView view = nullptr;
    GPU::TextureFormat format = GPU::TextureFormat::Undefined;
    WGPUCompositeAlphaMode alpha = WGPUCompositeAlphaMode_Auto;
    uint32_t width = 0, height = 0;
    bool configured = false;
    ~State() {
        if (view) wgpuTextureViewRelease(view);
        if (texture) wgpuTextureRelease(texture);
        if (surface) {
            if (configured) wgpuSurfaceUnconfigure(surface);
            wgpuSurfaceRelease(surface);
        }
        if (device) wgpuDeviceRelease(device);
    }
};

Surface::Surface(Lattice::Node& node, const Desc& desc) : state_(std::make_unique<State>()) {
    auto device = node.requireParent<Device>();
    state_->windowOwner = desc.window.owner;
    state_->device = device->native();
    wgpuDeviceAddRef(state_->device);
    state_->surface = createSurface(device->instance(), desc.window);
    if (!state_->surface)
        throw Lattice::Exception("WGPU::Surface", "failed to create surface");
    WGPUSurfaceCapabilities caps{};
    if (wgpuSurfaceGetCapabilities(state_->surface, device->adapter(), &caps) != WGPUStatus_Success)
        throw Lattice::Exception("WGPU::Surface", "device cannot present to this window");
    for (size_t i = 0; i < caps.formatCount; ++i) {
        if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm) {
            state_->format = GPU::TextureFormat::BGRA8Unorm;
            break;
        }
        if (caps.formats[i] == WGPUTextureFormat_RGBA8Unorm)
            state_->format = GPU::TextureFormat::RGBA8Unorm;
    }
    if (caps.alphaModeCount) state_->alpha = caps.alphaModes[0];
    wgpuSurfaceCapabilitiesFreeMembers(caps);
    if (state_->format == GPU::TextureFormat::Undefined)
        throw Lattice::Exception("WGPU::Surface", "no supported presentation format");
}

Surface::~Surface() = default;
GPU::TextureFormat Surface::format() const { return state_->format; }
WGPUTextureView Surface::view() const { return state_->view; }
WGPUDevice Surface::device() const { return state_->device; }

void Surface::releaseFrame() {
    if (state_->view) { wgpuTextureViewRelease(state_->view); state_->view = nullptr; }
    if (state_->texture) { wgpuTextureRelease(state_->texture); state_->texture = nullptr; }
}

void Surface::resize(uint32_t width, uint32_t height) {
    if (state_->configured && width == state_->width && height == state_->height) return;
    releaseFrame();
    state_->width = width;
    state_->height = height;
    if (!width || !height) {
        if (state_->configured) wgpuSurfaceUnconfigure(state_->surface);
        state_->configured = false;
        return;
    }
    WGPUSurfaceConfiguration config{};
    config.device = state_->device;
    config.format = nativeFormat(state_->format);
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = width;
    config.height = height;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = state_->alpha;
    wgpuSurfaceConfigure(state_->surface, &config);
    state_->configured = true;
}

bool Surface::acquire() {
    releaseFrame();
    if (!state_->configured) return false;
    WGPUSurfaceTexture current{};
    wgpuSurfaceGetCurrentTexture(state_->surface, &current);
    state_->texture = current.texture;
    switch (current.status) {
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
            if (!current.texture) throw Lattice::Exception("WGPU::Surface", "empty frame texture");
            state_->view = wgpuTextureCreateView(current.texture, nullptr);
            if (!state_->view) { releaseFrame(); throw Lattice::Exception("WGPU::Surface", "failed to create frame view"); }
            return true;
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        case WGPUSurfaceGetCurrentTextureStatus_Lost:
            releaseFrame();
            state_->configured = false;
            resize(state_->width, state_->height);
            return false;
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
            releaseFrame();
            return false;
        default:
            releaseFrame();
            throw Lattice::Exception("WGPU::Surface", "acquire failed ({})", static_cast<int>(current.status));
    }
}
void Surface::present() {
    if (!state_->view) throw Lattice::Exception("WGPU::Surface", "no acquired frame");
    const auto status = wgpuSurfacePresent(state_->surface);
    releaseFrame();
    if (status != WGPUStatus_Success) throw Lattice::Exception("WGPU::Surface", "present failed");
}

Pipeline::Pipeline(Lattice::Node& node, const Desc& desc) {
    auto device = node.requireParent<Device>();
    auto* shader = dynamic_cast<Shader*>(desc.shader);
    if (!shader || shader->device() != device->native()) throw Lattice::Exception("WGPU::Pipeline", "expected WGPU shader");
    device_ = device->native();
    WGPUColorTargetState color{};
    color.format = nativeFormat(desc.colorFormat);
    color.writeMask = WGPUColorWriteMask_All;
    WGPUFragmentState fragment{};
    fragment.module = shader->native();
    fragment.entryPoint = toWGPU(desc.fragmentEntry);
    fragment.targetCount = 1;
    fragment.targets = &color;
    WGPURenderPipelineDescriptor pipeline{};
    pipeline.vertex.module = shader->native();
    pipeline.vertex.entryPoint = toWGPU(desc.vertexEntry);
    pipeline.fragment = &fragment;
    pipeline.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline.multisample.count = 1;
    pipeline.multisample.mask = 0xFFFFFFFF;
    pipeline_ = wgpuDeviceCreateRenderPipeline(device_, &pipeline);
    if (!pipeline_) throw Lattice::Exception("WGPU::Pipeline", "failed to create pipeline");
}

Pipeline::~Pipeline() { if (pipeline_) wgpuRenderPipelineRelease(pipeline_); }

}
