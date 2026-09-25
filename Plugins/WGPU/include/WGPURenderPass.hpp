#pragma once

#include <span>
#include <webgpu/webgpu.h>

#include "CommandList.hpp"

namespace WGPU {

class Pipeline;
class Surface;

class BindingSet final : public GPU::BindingSet {
public:
    BindingSet(
        WGPUDevice device,
        Pipeline& pipeline,
        uint32_t group,
        std::span<const GPU::Binding> bindings
    );
    ~BindingSet() override;

    WGPUBindGroup native() const noexcept;

private:
    WGPUBindGroup bindGroup_ = nullptr;
};

class RenderPass final : public GPU::RenderPass {
public:
    ~RenderPass() override;

    bool active() const noexcept;
    void begin(WGPUCommandEncoder encoder, Surface& surface, GPU::Color clear);

    void setViewport(GPU::Rect rect) override;
    void setScissor(GPU::Rect rect) override;
    void setPipeline(GPU::Pipeline& pipeline) override;
    void setBindings(uint32_t group, GPU::BindingSet& bindings) override;
    void draw(uint32_t vertexCount, uint32_t firstVertex = 0) override;
    void end() override;

private:
    void requireActive() const;

    WGPURenderPassEncoder pass_ = nullptr;
    WGPURenderPipeline pipeline_ = nullptr;
    WGPUDevice device_ = nullptr;
};

}
