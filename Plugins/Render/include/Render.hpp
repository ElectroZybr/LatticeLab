#pragma once

#include <Lattice/Kernel/Component.hpp>

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Node.hpp>

// Plugin dependences

class WindowAPI;
namespace WGPU {
    class WGPU;
}

class Render : public Lattice::Component {
    static constexpr std::string_view tag = "Render";
public:
    explicit Render(Lattice::Node& renderer);
    void configure(Lattice::Node& renderer);

    void setup();
    void frame();
    ~Render();

private:
    struct FrameState;
    std::unique_ptr<FrameState> frameState;

    void ensureSurface(WindowAPI& window);
    void resize(uint32_t w, uint32_t h);
    void releaseFrameResources();

    Slot<WindowAPI> window_;
    Ref<WGPU::WGPU> gpu_;
};