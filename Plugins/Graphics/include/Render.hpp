#pragma once
#include <Lattice/Kernel/Node.hpp>
#include <Graphics/include/Device.hpp>
#include <Graphics/include/Surface.hpp>
#include <GPU/include/Shader.hpp>
#include <Graphics/include/RenderPipeline.hpp>

class WindowAPI;

class Render : public Lattice::Component {
public:
    explicit Render(Lattice::Node& renderer);
    void configure(Lattice::Node& renderer);
    void frame();
    void releaseFrameResources();
private:
    Mount<Graphics::Device> device_;
    Slot<WindowAPI> window_;
    Ref<Graphics::Surface> surface_;
    Ref<GPU::Shader> shader_;
    Ref<Graphics::RenderPipeline> pipeline_;
    NativeWindow native_;
    std::string resourceName_;
};
