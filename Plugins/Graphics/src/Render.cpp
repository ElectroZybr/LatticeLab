#include "Render.hpp"
#include "WindowAPI.hpp"
#include <fstream>
#include <iterator>

Render::Render(Lattice::Node& renderer)
    : device_(renderer.mount<Graphics::Device>()),
      resourceName_(std::format("render-{}", renderer.getId())) {}

void Render::configure(Lattice::Node& renderer) {
    window_ = renderer.find<WindowAPI>();
    if (!window_ || window_->shouldClose()) return;
    const auto native = window_->native();
    if (native.kind == NativeWindow::Kind::None) return;
    if (surface_ && native_ != native) releaseFrameResources();
    if (!surface_) {
        try {
            auto& branch = device_.branch();
            surface_ = branch.add<Graphics::Surface>(resourceName_, Graphics::SurfaceDesc{native});
            std::ifstream file("Plugins/Graphics/src/shaders/circle.wgsl");
            if (!file) throw Lattice::Exception("Render", "cannot read circle.wgsl");
            GPU::ShaderDesc shader;
            shader.source.assign(std::istreambuf_iterator<char>(file), {});
            shader_ = branch.add<GPU::Shader>(resourceName_, shader);
            pipeline_ = branch.add<Graphics::RenderPipeline>(resourceName_,
                Graphics::RenderPipelineDesc{shader_.getPtr(), surface_->format()});
            native_ = native;
        } catch (...) {
            releaseFrameResources();
            throw;
        }
    }
    const auto size = window_->framebufferSize();
    surface_->resize(size.x > 0 ? uint32_t(size.x) : 0, size.y > 0 ? uint32_t(size.y) : 0);
}

void Render::frame() {
    if (!window_ || window_->shouldClose()) return;
    if (!surface_) return;
    const auto size = window_->framebufferSize();
    surface_->resize(size.x > 0 ? uint32_t(size.x) : 0, size.y > 0 ? uint32_t(size.y) : 0);
    if (!surface_->acquire()) return;
    try {
        auto commands = device_->createRenderCommandList();
        commands->draw(*surface_, *pipeline_, {}, 6);
        commands->submit();
        surface_->present();
    } catch (...) {
        surface_->releaseFrame();
        throw;
    }
}

void Render::releaseFrameResources() {
    auto& branch = device_.branch();
    branch.dumpTree();
    branch.remove<Graphics::RenderPipeline>(resourceName_);
    branch.remove<GPU::Shader>(resourceName_);
    branch.remove<Graphics::Surface>(resourceName_);
    pipeline_ = {};
    shader_ = {};
    surface_ = {};
    native_ = {};
}
