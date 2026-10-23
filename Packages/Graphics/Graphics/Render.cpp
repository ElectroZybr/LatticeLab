#include "Render.hpp"

#include <WGPU/CommandList.hpp>

Render::Render(WGPU::Device& device, WindowAPI& window, WGPU::Surface& surface,
               std::vector<Viewport*> viewports, std::vector<TransformController*> controllers)
    : device_(device), window_(window), surface_(surface),
      viewports_(std::move(viewports)), controllers_(std::move(controllers)) {}

void Render::frame(float) {
    if (window_.shouldClose())
        return;

    const auto size = window_.framebufferSize();
    const glm::uvec2 framebuffer{size.x > 0 ? uint32_t(size.x) : 0u, size.y > 0 ? uint32_t(size.y) : 0u};

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
