#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <wgpu/Resources.hpp>
#include <wgpu/Types.hpp>

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

/// Pixel coordinates, measured from the top-left of the surface.
struct Rect {
    uint32_t x = 0, y = 0, width = 0, height = 0;
};

/// Faces a pipeline discards; front faces are counter-clockwise.
enum class CullMode { None, Back };
enum class DepthCompare { Less, LessEqual, Greater, Always };

struct RenderPipelineDesc {
    Wgpu::TextureFormat colorFormat = Wgpu::TextureFormat::BGRA8Unorm;
    /// Undefined: no depth test. Otherwise the render pass must have a depth target.
    Wgpu::TextureFormat depthFormat = Wgpu::TextureFormat::Undefined;
    bool depthWrite = true;
    DepthCompare depthCompare = DepthCompare::Less;
    /// Alpha blending of the color target (transparent objects; usually without depth writes).
    bool blend = false;
    CullMode cull = CullMode::None;
    /// Bind group layouts, group i = layouts[i]; shared between pipelines (the camera group).
    std::vector<Wgpu::BindingLayout*> layouts;
    std::string vertexEntry = "vs";
    std::string fragmentEntry = "fs";
    std::string label;
};
