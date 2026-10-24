#include <cstdint>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Surface.hpp"

#include <Lattice/Tools/Exception.hpp>
#include <Wgpu/Native.hpp>

using namespace Wgpu;

namespace {

WGPUTextureFormat presentationFormat(TextureFormat format) {
    if (format != TextureFormat::BGRA8Unorm && format != TextureFormat::RGBA8Unorm)
        throw Lattice::Exception<Surface>("unsupported presentation format");
    return nativeFormat(format);
}

WGPUSurface createNativeSurface(WGPUInstance instance, const NativeWindow& window) {
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
        default: throw Lattice::Exception<Surface>("unsupported native window");
    }
}

}

struct Surface::State {
    std::shared_ptr<void> windowOwner;
    WGPUSurface surface = nullptr;
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUTexture texture = nullptr;
    WGPUTextureView view = nullptr;
    TextureFormat format = TextureFormat::Undefined;
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

void Surface::attach(const NativeWindow& window) {
    if (window.kind == NativeWindow::Kind::None)
        throw Lattice::Exception<Surface>("native window is required");

    releaseFrame();
    releaseSurface();

    state_->windowOwner = window.owner;
    state_->format = TextureFormat::Undefined;

    state_->surface = createNativeSurface(state_->instance, window);

    if (!state_->surface)
        throw Lattice::Exception<Surface>("failed to create surface");

    WGPUSurfaceCapabilities caps{};

    if (wgpuSurfaceGetCapabilities(state_->surface, state_->adapter, &caps) != WGPUStatus_Success) {
        releaseSurface();
        throw Lattice::Exception<Surface>("device cannot present to this window");
    }

    for (size_t i = 0; i < caps.formatCount; ++i) {
        if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm) {
            state_->format = TextureFormat::BGRA8Unorm;
            break;
        }

        if (caps.formats[i] == WGPUTextureFormat_RGBA8Unorm)
            state_->format = TextureFormat::RGBA8Unorm;
    }

    if (caps.alphaModeCount)
        state_->alpha = caps.alphaModes[0];

    wgpuSurfaceCapabilitiesFreeMembers(caps);

    if (state_->format == TextureFormat::Undefined) {
        releaseSurface();
        throw Lattice::Exception<Surface>("no supported presentation format");
    }
}

Surface::Surface(Device& device, const NativeWindow& window)
    : Tracked(device, "Surface", "window", 0), state_(std::make_unique<State>()) {
    state_->device = device.native();
    state_->instance = device.instance();
    state_->adapter = device.adapter();
    wgpuDeviceAddRef(state_->device);
    attach(window);
}

void Surface::releaseSurface() {
    if (!state_->surface)
        return;

    wgpuSurfaceRelease(state_->surface);
    state_->surface = nullptr;
    state_->windowOwner.reset();
    state_->format = TextureFormat::Undefined;
}

Surface::~Surface() = default;

TextureFormat Surface::format() const { return state_->format; }

WGPUTextureView Surface::view() const noexcept {
    return state_->view;
}

void Surface::releaseFrame() {
    if (state_->view) {
        wgpuTextureViewRelease(state_->view);
        state_->view = nullptr;
    }
    if (state_->texture) {
        wgpuTextureRelease(state_->texture);
        state_->texture = nullptr;
    }
}

void Surface::resize(uint32_t width, uint32_t height) {
    if (state_->configured && width == state_->width && height == state_->height)
        return;

    releaseFrame();
    state_->width = width;
    state_->height = height;

    if (!width || !height) {
        if (state_->configured)
            wgpuSurfaceUnconfigure(state_->surface);

        state_->configured = false;
        return;
    }

    WGPUSurfaceConfiguration config{};
    config.device = state_->device;
    config.format = presentationFormat(state_->format);
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
    if (!state_->configured)
        return false;

    WGPUSurfaceTexture current{};
    wgpuSurfaceGetCurrentTexture(state_->surface, &current);
    state_->texture = current.texture;
    switch (current.status) {
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
            if (!current.texture)
                throw Lattice::Exception<Surface>("empty frame texture");

            state_->view = wgpuTextureCreateView(current.texture, nullptr);
            if (!state_->view) {
                releaseFrame();
                throw Lattice::Exception<Surface>("failed to create frame view");
            }

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
            throw Lattice::Exception<Surface>("acquire failed ({})", static_cast<int>(current.status));
    }
}

void Surface::present() {
    if (!state_->view)
        throw Lattice::Exception<Surface>("no acquired frame");

    const auto status = wgpuSurfacePresent(state_->surface);
    releaseFrame();
    if (status != WGPUStatus_Success)
        throw Lattice::Exception<Surface>("present failed");
}
