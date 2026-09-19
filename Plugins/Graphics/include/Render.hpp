#pragma once

#include "Viewport.hpp"
#include <Lattice/Kernel/Node.hpp>
#include <GPU/include/Device.hpp>
#include <GPU/include/Surface.hpp>
#include <GPU/include/Shader.hpp>
#include <GPU/include/Pipeline.hpp>
#include "Lattice/Kernel/RefSlot.hpp"

class WindowAPI;

class Render : public Lattice::Component {
public:
    explicit Render(Lattice::Node& renderer);
    void configure(Lattice::Node& renderer);
    void frame();
    void releaseFrameResources();
private:
    Mount<GPU::Device> device_;
    Slot<WindowAPI> window_;
    Ref<GPU::Surface> surface_;
    Ref<GPU::Shader> shader_;
    Ref<GPU::Pipeline> pipeline_;
    NativeWindow native_;
    std::string resourceName_;

    Children<Viewport> viewports_;
};
