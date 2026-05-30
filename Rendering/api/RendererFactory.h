#pragma once

#include <memory>

#include <webgpu/webgpu.hpp>

#include "Rendering/2d/Renderer2DWGPU.h"
#include "Rendering/3d/Renderer3DWGPU.h"

class BaseRenderer;

namespace Rendering::API {
    enum class RendererKind : unsigned char {
        Renderer2D = 0,
        Renderer3D = 1,
    };

    inline std::unique_ptr<BaseRenderer> createRenderer(RendererKind kind, wgpu::TextureFormat surfaceFormat) {
        switch (kind) {
        case RendererKind::Renderer2D:
            return std::make_unique<Renderer2DWGPU>(surfaceFormat);
        case RendererKind::Renderer3D:
            return std::make_unique<Renderer3DWGPU>(surfaceFormat);
        }

        return nullptr;
    }
}
