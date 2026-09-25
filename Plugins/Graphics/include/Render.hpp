#pragma once

#include "Viewport.hpp"
#include <Lattice/Kernel/NodeViews.hpp>
#include <GPU/include/Device.hpp>
#include <GPU/include/Surface.hpp>
#include <GPU/include/Shader.hpp>
#include <GPU/include/Pipeline.hpp>
#include <GPU/include/Buffer.hpp>

class WindowAPI;

class Render : public Lattice::Component {
public:
    explicit Render(NodeBuild renderer);
    void configure(NodeConfigure renderer);
    ~Render();
    void frame(float dt = 0.0f);
    void releaseFrameResources();
private:
    Slot<WindowAPI> window_;
    Ref<GPU::Surface> surface_;
    Ref<GPU::Device> device_;
    NativeWindow native_;
    std::string resourceName_;
    Children<Viewport> viewports_;
};
