#pragma once

#include <memory>

#include <Lattice/Kernel/NodeViews.hpp>

#include <GPU/include/Buffer.hpp>
#include <GPU/include/Device.hpp>
#include <GPU/include/Pipeline.hpp>
#include <GPU/include/Shader.hpp>
#include <GPU/include/Texture.hpp>

#include "Viewport.hpp"

class Camera;
class TransformController;

class TestViewport final : public Viewport {
public:
    struct Desc {
        GPU::TextureFormat colorFormat = GPU::TextureFormat::BGRA8Unorm;
    };

    explicit TestViewport(NodeBuild node, const Desc& desc);
    void render(GPU::RenderPass& pass, glm::uvec2 surfaceSize) override;

private:
    Ref<GPU::Device> device_;
    Ref<GPU::Buffer> uniform_;
    Ref<GPU::Shader> shader_;
    Ref<GPU::Pipeline> pipeline_;
    Ref<Camera> camera_;
    Ref<TransformController> controller_;
    std::unique_ptr<GPU::BindingSet> bindings_;
};
