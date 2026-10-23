#pragma once

#include <filesystem>

#include <WGPU/Resources.hpp>

#include "RenderLayout.hpp"
#include "SceneObject.hpp"

/// A unit disc at its transform: the simplest scene object, owns its GPU resources.
class TestObject final : public SceneObject {
public:
    TestObject(RenderLayout& layout, const std::filesystem::path& shader);
    void render(WGPU::RenderPass& pass, const ViewState& view) override;

private:
    WGPU::Device& device_;
    WGPU::BindingLayout modelLayout_;
    WGPU::Buffer model_;
    WGPU::BindingSet modelGroup_;
    WGPU::Shader shader_;
    WGPU::Pipeline pipeline_;
};
