#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include "Lattice/Kernel/Node.hpp"
#include <GPU/include/CommandList.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.hpp"
#include "Render.hpp"
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

        auto render = node.requireParent<Render>();
        // GPU::Binding binding{};
        // binding.binding = 0;
        // binding.buffer = uniform.getPtr();
        // binding.offset = 0;
        // binding.size = 16;

        // bindingSet = device_->createBindingSet(*pipeline_, 0, std::span(&binding, 1));
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
        if (!camera_ || position_.x >= surfaceSize.x || position_.y >= surfaceSize.y)
            return;

        const auto available = surfaceSize - position_;
        const auto requested = sizeMode_ == ViewportSizeMode::Fill ? available : size_;
        const glm::uvec2 extent{
            std::min(requested.x, available.x),
            std::min(requested.y, available.y)
        };

        if (!extent.x || !extent.y)
            return;

        const GPU::Rect rect{position_.x, position_.y, extent.x, extent.y};
        const auto matrix = camera_->viewProjection(float(extent.x) / float(extent.y));

        // pass.setViewport(rect);
        // pass.setScissor(rect);
        // pass.setPipeline(pipeline);
        // pass.setBindings(0, *cameraBindings);
        // pass.draw(6);
    }

private:
    Ref<Camera> camera_;
    glm::uvec2 position_{};
    glm::uvec2 size_{1280, 720};
    ViewportSizeMode sizeMode_ = ViewportSizeMode::Fill;
    GPU::BindingSet bindingSet{};
};