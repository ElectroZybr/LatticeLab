#pragma once

#include "Surface.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <span>

namespace GPU {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

// Pixel coordinates, measured from the top-left of the surface.
struct Rect {
    uint32_t x = 0, y = 0, width = 0, height = 0;
};

struct Buffer;

struct Binding {
    uint32_t binding = 0;
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
};

class BindingSet {
public:
    virtual ~BindingSet() = default;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void setViewport(Rect rect) = 0;
    virtual void setScissor(Rect rect) = 0;
    virtual void setBindings(uint32_t group, BindingSet& bindings) = 0;
    virtual void setPipeline(Pipeline& pipeline) = 0;
    // virtual void setUniform(uint32_t, uint32_t, std::span<const std::byte>) {}
    virtual void draw(uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
    // virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex = 0) = 0;
    virtual void end() = 0;
};

class CommandList {
public:
    virtual ~CommandList() = default;

    virtual RenderPass& beginRenderPass(Surface& surface, Color clear = {}) = 0;

    virtual void submit() = 0;
};

}
