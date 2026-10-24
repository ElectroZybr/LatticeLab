#pragma once

#include <array>

#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>

/// What every pipeline drawing into one surface agrees on: color and depth formats
/// and the camera group (bind group 0, owned by each viewport). Shared by the
/// viewports and scene objects of a window.
class RenderLayout final {
public:
    RenderLayout(Wgpu::Device& device, Wgpu::TextureFormat colorFormat,
                 Wgpu::TextureFormat depthFormat = Wgpu::TextureFormat::Depth32Float)
        : device_(device), colorFormat_(colorFormat), depthFormat_(depthFormat),
          camera_(device, CameraEntries, "camera") {}

    Wgpu::Device& device() const noexcept { return device_; }
    Wgpu::TextureFormat colorFormat() const noexcept { return colorFormat_; }
    Wgpu::TextureFormat depthFormat() const noexcept { return depthFormat_; }
    /// Group 0 of every scene pipeline, owned by each viewport: binding 0 — the camera
    /// uniform (CameraUniform), binding 1 — the lights of the scene (LightsHeader and
    /// LightRecord, Viewport.hpp). Pipelines that do not light ignore binding 1.
    Wgpu::BindingLayout& camera() noexcept { return camera_; }

private:
    static constexpr std::array<Wgpu::LayoutEntry, 2> CameraEntries{{
        {.binding = 0, .type = Wgpu::BindingType::Uniform},
        {.binding = 1, .type = Wgpu::BindingType::ReadOnlyStorage},
    }};

    Wgpu::Device& device_;
    Wgpu::TextureFormat colorFormat_;
    Wgpu::TextureFormat depthFormat_;
    Wgpu::BindingLayout camera_;
};
