#pragma once

#include <memory>

#include <webgpu/webgpu.h>
#include <Shell/NativeWindow.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/Types.hpp>

/// Presentation target of one native window: the renderer's surface (an internal
/// resource of Render, not a tree node).
class Surface final : Wgpu::Tracked {
public:
    Surface(Wgpu::Device& device, const NativeWindow& window);
    ~Surface();

    Wgpu::TextureFormat format() const;
    void attach(const NativeWindow& window);
    void resize(uint32_t width, uint32_t height);
    bool acquire();
    void present();
    void releaseFrame();

    WGPUTextureView view() const noexcept;
    Wgpu::Device& device() const noexcept { return Tracked::device(); }

private:
    void releaseSurface();

    struct State;
    std::unique_ptr<State> state_;
};
