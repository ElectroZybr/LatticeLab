#pragma once

#include <filesystem>

#include <Wgpu/Resources.hpp>

#include "RenderLayout.hpp"
#include "RenderPipeline.hpp"
#include "SceneObject.hpp"

/// A unit disc at its transform: the simplest scene object, owns its GPU resources.
class TestObject final : public SceneObject {
public:
    TestObject(RenderLayout& layout, const std::filesystem::path& shader);
    void render(RenderPass& pass, const ViewState& view) override;

private:
    Wgpu::Device& device_;
    Wgpu::BindingLayout modelLayout_;
    Wgpu::Buffer model_;
    Wgpu::BindingSet modelGroup_;
    Wgpu::Shader shader_;
    RenderPipeline pipeline_;
};
