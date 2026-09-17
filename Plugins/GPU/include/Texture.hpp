#pragma once

#include <cstdint>

namespace GPU {

enum class TextureFormat {
    Undefined,

    R8Unorm,
    RG8Unorm,
    RGBA8Unorm,
    BGRA8Unorm,

    R16Float,
    RG16Float,
    RGBA16Float,

    R32Float,
    RG32Float,
    RGBA32Float,

    Depth16,
    Depth24,
    Depth24Stencil8,
    Depth32Float
};

enum class TextureUsage : uint32_t {
    None            = 0,
    Sampled         = 1 << 0,
    Storage         = 1 << 1,
    RenderTarget    = 1 << 2,
    DepthStencil    = 1 << 3,
    CopySource      = 1 << 4,
    CopyDestination = 1 << 5
};

struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::None;
};

struct Texture {
    virtual ~Texture() = default;
};

}