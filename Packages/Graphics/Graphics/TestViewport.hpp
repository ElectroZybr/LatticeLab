#pragma once

#include <filesystem>

#include <WGPU/Device.hpp>
#include <WGPU/Resources.hpp>

#include "Camera.hpp"
#include "Viewport.hpp"

/// Draws a test shader through one camera. Owns its GPU resources.
class TestViewport final : public Viewport {
public:
    TestViewport(WGPU::Device& device, WGPU::TextureFormat colorFormat, Camera& camera,
                 const std::filesystem::path& shader);
    void render(WGPU::RenderPass& pass, glm::uvec2 surfaceSize) override;

private:
    WGPU::Device& device_;
    Camera& camera_;
    WGPU::Buffer uniform_;
    WGPU::Shader shader_;
    WGPU::Pipeline pipeline_;
    WGPU::BindingSet bindings_;
};
