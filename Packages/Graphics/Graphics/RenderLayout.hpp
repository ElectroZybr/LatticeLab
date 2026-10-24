#pragma once

#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>

/// What every pipeline drawing into one surface agrees on: color and depth formats
/// and the camera group (bind group 0, owned by each viewport). Shared by the
/// viewports and scene objects of a window.
class RenderLayout final {
public:
    RenderLayout(Wgpu::Device& device, Wgpu::TextureFormat colorFormat,
                 Wgpu::TextureFormat depthFormat = Wgpu::TextureFormat::Depth32Float);

    Wgpu::Device& device() const noexcept { return device_; }
    Wgpu::TextureFormat colorFormat() const noexcept { return colorFormat_; }
    Wgpu::TextureFormat depthFormat() const noexcept { return depthFormat_; }
    /// Group 0 of every scene pipeline: binding 0 — the camera uniform (CameraUniform).
    Wgpu::BindingLayout& camera() noexcept { return camera_; }

private:
    Wgpu::Device& device_;
    Wgpu::TextureFormat colorFormat_;
    Wgpu::TextureFormat depthFormat_;
    Wgpu::BindingLayout camera_;
};
