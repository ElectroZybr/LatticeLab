#pragma once

#include <Lattice/Kernel/NodeViews.hpp>

#include <GPU/include/Device.hpp>
#include <GPU/include/Surface.hpp>

#include "Viewport.hpp"
#include "WindowAPI.hpp"

class Render : public Lattice::Component {
public:
    explicit Render(NodeBuild node);
    void configure(NodeConfigure node);
    void frame(float dt = 0.0f);

private:
    Slot<WindowAPI> window_;
    Ref<GPU::Device> device_;
    Ref<GPU::Surface> surface_;
    Children<Viewport> viewports_;
};
