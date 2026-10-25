#include "Render.hpp"

#include <algorithm>

#include "RenderPass.hpp"

Render::Render(Wgpu::Device& device, WindowAPI& window)
    : device_(device), window_(window), surface_(device, window.native()),
      layout_(device, surface_.format()) {}

void Render::setViewports(std::vector<Viewport*> viewports) {
    viewports_ = std::move(viewports);
    // Draw order is by layer; the stable sort keeps tree order within a layer.
    std::ranges::stable_sort(viewports_, {}, &Viewport::layer);
    if (std::ranges::find(viewports_, hovered_) == viewports_.end())
        hovered_ = nullptr;
}

void Render::routeCursor(glm::uvec2 surfaceSize) {
    Viewport* target = nullptr;
    for (auto it = viewports_.rbegin(); it != viewports_.rend() && !target; ++it)
        if ((*it)->contains(cursor_, surfaceSize))
            target = *it;
    for (Viewport* viewport : viewports_)
        viewport->hover(viewport == target
            ? std::optional{cursor_ - glm::vec2(viewport->position())}
            : std::nullopt);
    if (target != hovered_) {
        hovered_ = target;
        if (onHover_) onHover_(target);
    }
}

void Render::frame(float) {
    if (window_.shouldClose())
        return;

    const auto size = window_.framebufferSize();
    const glm::uvec2 framebuffer{size.x > 0 ? uint32_t(size.x) : 0u, size.y > 0 ? uint32_t(size.y) : 0u};

    routeCursor(framebuffer);
    for (Viewport* viewport : viewports_)
        if (viewport->controller)
            viewport->controller->update();

    surface_.resize(framebuffer.x, framebuffer.y);
    if (!framebuffer.x || !framebuffer.y)
        return;
    if (!depth_ || depth_->width() != framebuffer.x || depth_->height() != framebuffer.y)
        depth_ = std::make_unique<Wgpu::Texture>(device_, Wgpu::TextureDesc{
            .width = framebuffer.x, .height = framebuffer.y, .format = layout_.depthFormat(),
            .usage = Wgpu::TextureUsage::RenderAttachment, .label = "render/depth"});
    if (!surface_.acquire())
        return;

    try {
        Wgpu::CommandList commands{device_};
        RenderPass pass{commands, surface_, {0.1f, 0.2f, 0.3f, 1.0f}, depth_.get()};
        for (Viewport* viewport : viewports_)
            viewport->render(pass, framebuffer);
        pass.end();
        commands.submit();
        surface_.present();
    } catch (...) {
        surface_.releaseFrame();
        throw;
    }
}
