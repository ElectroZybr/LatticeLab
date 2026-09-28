#include "Render.hpp"
#include <Lattice/Tools/Exception.hpp>
#include "Shell/src/glfwWindow/glfwWindow.hpp"


Render::Render(NodeBuild node) {
    window_ = node.addSlot<WindowAPI>();

    auto mounted = node.mount<GPU::Device>();
    device_ = mounted.ref();
    surface_ = mounted.addLocal<GPU::Surface>("surface");
}

void Render::configure(NodeConfigure node) {
    window_.choice<glfwWindow>();
    viewports_ = node.children<Viewport>();

    if (!window_)
        throw Lattice::Exception("Render", "window is required");

    const NativeWindow native = window_->native();
    if (native.kind == NativeWindow::Kind::None)
        throw Lattice::Exception("Render", "window has no native handle");

    surface_->attach(native);
}

void Render::frame(float) {
    if (!window_ || window_->shouldClose() || !device_ || !surface_)
        return;

    const auto size = window_->framebufferSize();
    const glm::uvec2 framebuffer{
        size.x > 0 ? uint32_t(size.x) : 0,
        size.y > 0 ? uint32_t(size.y) : 0
    };

    surface_->resize(framebuffer.x, framebuffer.y);
    if (!surface_->acquire())
        return;

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
