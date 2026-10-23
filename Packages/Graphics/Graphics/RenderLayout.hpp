#pragma once

#include <WGPU/Device.hpp>
#include <WGPU/Resources.hpp>

/// What every pipeline drawing into one surface agrees on: color and depth formats
/// and the camera group (bind group 0, owned by each viewport). Shared by the
/// viewports and scene objects of a window.
class RenderLayout final {
public:
    RenderLayout(WGPU::Device& device, WGPU::TextureFormat colorFormat,
                 WGPU::TextureFormat depthFormat = WGPU::TextureFormat::Depth32Float);

    WGPU::Device& device() const noexcept { return device_; }
    WGPU::TextureFormat colorFormat() const noexcept { return colorFormat_; }
    WGPU::TextureFormat depthFormat() const noexcept { return depthFormat_; }
    /// Group 0 of every scene pipeline: binding 0 — the camera uniform (CameraUniform).
    WGPU::BindingLayout& camera() noexcept { return camera_; }

private:
    WGPU::Device& device_;
    WGPU::TextureFormat colorFormat_;
    WGPU::TextureFormat depthFormat_;
    WGPU::BindingLayout camera_;
};
