#include <Lattice/Kernel/NodeViews.hpp>
#include "Shell/src/glfwWindow/glfwWindow.hpp"

#include "Render.hpp"
#include "Viewport.hpp"
#include "WindowAPI.hpp"


Render::Render(NodeBuild renderer) {
    auto deviceBuild = renderer.mount<GPU::Device>();
    device_ = deviceBuild.ref();
    resourceName_ = std::format("render-{}", renderer.id());
    surface_ = deviceBuild.add<GPU::Surface>(resourceName_);
    window_ = renderer.addSlot<WindowAPI>();
}

void Render::configure(NodeConfigure renderer) {
    viewports_ = renderer.children<Viewport>();
    if (!window_.exists()) window_.choice<glfwWindow>();
}

void Render::frame(float) {
    if (!window_ || window_->shouldClose()) return;
    if (!surface_) return;
    const auto size = window_->framebufferSize();
    const glm::uvec2 framebuffer{size.x > 0 ? uint32_t(size.x) : 0, size.y > 0 ? uint32_t(size.y) : 0};
    surface_->resize(framebuffer.x, framebuffer.y);
    if (!surface_->acquire()) return;
    try {
        auto commands = device_->createCommandList();
        auto& pass = commands->beginRenderPass(*surface_, {0.1f, 0.2f, 0.3f, 1.0f});
        for (Viewport* viewport : viewports_)
            viewport->render(pass, framebuffer);
        pass.end();
        commands->submit();
        surface_->present();
    } catch (...) {
        surface_->releaseFrame();
        throw;
    }
}

Render::~Render() {
    releaseFrameResources();
}

void Render::releaseFrameResources() {
    // device_.remove<GPU::Pipeline>(resourceName_);
    // device_.remove<GPU::Shader>(resourceName_);
    // device_.remove<GPU::Surface>(resourceName_);
    // pipeline_ = {};
    // shader_ = {};
    // surface_ = {};
    // native_ = {};
}
