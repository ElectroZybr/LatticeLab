#pragma once

#include <cstdint>

#include <webgpu/webgpu.h>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/Resources.hpp>

#include "RenderPipeline.hpp"
#include "RenderTypes.hpp"
#include "Surface.hpp"

/// A render pass into an acquired surface, recorded into a command list. Begins on
/// construction, ends with end() or on destruction; the list submits after it ends.
class RenderPass final {
public:
    /// `depth`: optional depth target of the surface size, cleared to 1. `timestamps`:
    /// GPU time of the pass (Wgpu::Timer::renderPass), optional.
    RenderPass(Wgpu::CommandList& commands, Surface& surface, Color clear, Wgpu::Texture* depth = nullptr,
               const WGPURenderPassTimestampWrites* timestamps = nullptr);
    /// Into a texture (RenderAttachment) instead of a window: captures and benchmarks.
    RenderPass(Wgpu::CommandList& commands, Wgpu::Texture& target, Color clear, Wgpu::Texture* depth = nullptr,
               const WGPURenderPassTimestampWrites* timestamps = nullptr);
    ~RenderPass();
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    bool active() const noexcept { return pass_ != nullptr; }

    void setViewport(Rect rect);
    void setScissor(Rect rect);
    void setPipeline(RenderPipeline& pipeline);
    void setBindings(uint32_t group, Wgpu::BindingSet& bindings);
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0);
    void end();

private:
    void begin(WGPUTextureView target, Color clear, Wgpu::Texture* depth, const WGPURenderPassTimestampWrites* timestamps);
    void requireActive() const;

    Wgpu::CommandList& commands_;
    WGPURenderPassEncoder pass_ = nullptr;
    bool pipeline_ = false;
};
