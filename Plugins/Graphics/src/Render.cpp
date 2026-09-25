#include <fstream>
#include <iterator>
#include <Lattice/Kernel/NodeViews.hpp>

#include "Render.hpp"
#include "Viewport.hpp"
#include "WindowAPI.hpp"
#include "TransformController.hpp"

Render::Render(NodeBuild renderer) {
    device_ = renderer.mount<GPU::Device>();
    resourceName_ = std::format("render-{}", renderer.id());
    renderer.add<Viewport>("Main");
    
}

void Render::configure(NodeConfigure renderer) {
    window_ = renderer.find<WindowAPI>();
    viewports_ = renderer.children<Viewport>();
    if (!window_ || window_->shouldClose()) return;
    const auto native = window_->native();
    if (native.kind == NativeWindow::Kind::None) return;
    if (surface_ && native_ != native) releaseFrameResources();
    if (!surface_) {
        try {
            surface_ = device_.add<GPU::Surface>(resourceName_, GPU::SurfaceDesc{native});
            std::ifstream file("Plugins/Graphics/src/shaders/circle.wgsl");
            if (!file) throw Lattice::Exception("Render", "cannot read circle.wgsl");
            GPU::ShaderDesc shader;
            shader.source.assign(std::istreambuf_iterator<char>(file), {});
            shader_ = device_.add<GPU::Shader>(resourceName_, shader);
            pipeline_ = device_.add<GPU::Pipeline>(resourceName_, GPU::PipelineDesc{shader_.getPtr(), surface_->format()});
            native_ = native;
        } catch (...) {
            releaseFrameResources();
            throw;
        }
    }
    const auto size = window_->framebufferSize();
    surface_->resize(size.x > 0 ? uint32_t(size.x) : 0, size.y > 0 ? uint32_t(size.y) : 0);
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
            viewport->render(pass, *pipeline_, framebuffer);
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
    device_.remove<GPU::Pipeline>(resourceName_);
    device_.remove<GPU::Shader>(resourceName_);
    device_.remove<GPU::Surface>(resourceName_);
    pipeline_ = {};
    shader_ = {};
    surface_ = {};
    native_ = {};
}
