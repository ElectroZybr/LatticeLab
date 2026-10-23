#include "Render.hpp"

#include <algorithm>

#include <WGPU/CommandList.hpp>

Render::Render(WGPU::Device& device, WindowAPI& window, WGPU::Surface& surface,
               std::vector<Viewport*> viewports, std::vector<TransformController*> controllers)
    : device_(device), window_(window), surface_(surface),
      viewports_(std::move(viewports)), controllers_(std::move(controllers)) {
    // Draw order is by layer; the stable sort keeps composition order within a layer.
    std::ranges::stable_sort(viewports_, {}, &Viewport::layer);
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
}

void Render::frame(float) {
    if (window_.shouldClose())
        return;

    const auto size = window_.framebufferSize();
    const glm::uvec2 framebuffer{size.x > 0 ? uint32_t(size.x) : 0u, size.y > 0 ? uint32_t(size.y) : 0u};

    routeCursor(framebuffer);
    for (TransformController* controller : controllers_)
        controller->update();

    surface_.resize(framebuffer.x, framebuffer.y);
    if (!surface_.acquire())
        return;

    try {
        WGPU::CommandList commands{device_};
        auto& pass = commands.beginRenderPass(surface_, {0.1f, 0.2f, 0.3f, 1.0f});
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
