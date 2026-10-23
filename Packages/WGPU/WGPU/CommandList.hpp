#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

#include "Resources.hpp"

namespace WGPU {

class RenderPass final {
public:
    RenderPass() = default;
    ~RenderPass();
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    bool active() const noexcept { return pass_ != nullptr; }
    /// `depth`: optional depth target of the surface size, cleared to 1.
    void begin(WGPUCommandEncoder encoder, Surface& surface, Color clear, Texture* depth = nullptr);

    void setViewport(Rect rect);
    void setScissor(Rect rect);
    void setPipeline(Pipeline& pipeline);
    void setBindings(uint32_t group, BindingSet& bindings);
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0);
    void end();

private:
    void requireActive() const;

    WGPURenderPassEncoder pass_ = nullptr;
    bool pipeline_ = false;
    Device* device_ = nullptr;
};

/// One frame of recorded commands. Created per frame; not tracked as a resource.
class CommandList final {
public:
    explicit CommandList(Device& device);
    ~CommandList();
    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    RenderPass& beginRenderPass(Surface& surface, Color clear = {}, Texture* depth = nullptr);
    /// Offsets and size must be multiples of 4; buffers need CopySource / CopyDestination.
    void copyBuffer(Buffer& source, uint64_t sourceOffset, Buffer& target, uint64_t targetOffset, uint64_t size);
    void submit();

private:
    Device& device_;
    WGPUCommandEncoder encoder_ = nullptr;
    WGPUCommandBuffer commandBuffer_ = nullptr;
    RenderPass renderPass_;
    bool submitted_ = false;
};

}
