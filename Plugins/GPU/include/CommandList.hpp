#pragma once

#include "Surface.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <span>
#include <cstddef>

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

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void setViewport(Rect rect) = 0;
    virtual void setScissor(Rect rect) = 0;
    // Bind a group containing one uniform buffer; data is copied when recorded.
    virtual void setUniform(uint32_t group, uint32_t binding, std::span<const std::byte> data) = 0;
    virtual void setPipeline(Pipeline& pipeline) = 0;
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
