#pragma once

#include <algorithm>
#include <format>
#include <span>
#include <glm/glm.hpp>
#include "GPU/include/Buffer.hpp"
#include "GPU/include/Pipeline.hpp"
#include "Lattice/Kernel/Node.hpp"
#include <GPU/include/CommandList.hpp>
#include <GPU/include/Device.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.hpp"
#include "TransformController.hpp"

enum class ViewportSizeMode {
    Fixed,
    Fill
};

class Viewport : public Lattice::Component {
public:
    Viewport(Lattice::Node& node) {
        node.makeFocusScope();
        camera_ = node.add<Camera>("MainCamera").focus();
        camera_->setPosition({0.0f, 0.0f, 2.0f});
        node.add<TransformController>();
    }

    void configure(Lattice::Node& node) {
        if (!node.find<GPU::Device>().exists())
            return;
        device_ = node.require<GPU::Device>();
        GPU::BufferDesc desc{};
        desc.size = sizeof(glm::mat4);
        desc.usage = GPU::BufferUsage::Uniform | GPU::BufferUsage::CopyDestination;
        uniform_ = device_.add<GPU::Buffer>(std::format("uniform-{}", node.getId()), desc);
    }

    glm::uvec2 position() const noexcept { return position_; }
    void setPosition(glm::uvec2 position) noexcept { position_ = position; }

    glm::uvec2 size() const noexcept { return size_; }

    void setSize(glm::uvec2 size) noexcept {
        size_ = size;
        sizeMode_ = ViewportSizeMode::Fixed;
    }

    void fitSurface() noexcept {
        sizeMode_ = ViewportSizeMode::Fill;
    }

    void render(GPU::RenderPass& pass, GPU::Pipeline& pipeline, glm::uvec2 surfaceSize) {
        if (!camera_ || !device_ || !uniform_ || position_.x >= surfaceSize.x || position_.y >= surfaceSize.y)
            return;

        const auto available = surfaceSize - position_;
        const auto requested = sizeMode_ == ViewportSizeMode::Fill ? available : size_;
        const glm::uvec2 extent{
            std::min(requested.x, available.x),
            std::min(requested.y, available.y)
        };

        if (!extent.x || !extent.y)
            return;

        if (!bindings_) {
            GPU::Binding bind{};
            bind.binding = 0;
            bind.buffer = uniform_.getPtr();
            bind.size = sizeof(glm::mat4);
            bindings_ = device_->createBindingSet(pipeline, 0, std::span(&bind, 1));
        }

        const GPU::Rect rect{position_.x, position_.y, extent.x, extent.y};
        const auto matrix = camera_->viewProjection(float(extent.x) / float(extent.y));
        device_->writeBuffer(*uniform_, 0, std::as_bytes(std::span(&matrix, 1)));

        pass.setViewport(rect);
        pass.setScissor(rect);
        pass.setPipeline(pipeline);
        pass.setBindings(0, *bindings_);
        pass.draw(6);
    }

private:
    Ref<Camera> camera_;
    Ref<GPU::Device> device_;
    glm::uvec2 position_{};
    glm::uvec2 size_{1280, 720};
    ViewportSizeMode sizeMode_ = ViewportSizeMode::Fill;


    std::unique_ptr<GPU::BindingSet> bindings_;
    Ref<GPU::Buffer> uniform_;
};
