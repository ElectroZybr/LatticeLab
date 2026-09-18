#pragma once

#include <Lattice/Kernel/Component.hpp>

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
    Depth16Unorm,
    Depth24Plus,
    Depth24PlusStencil8,
    Depth32Float
};

enum class TextureUsage : uint32_t {
    None             = 0x00,
    CopySource       = 0x01,
    CopyDestination  = 0x02,
    TextureBinding   = 0x04,
    StorageBinding   = 0x08,
    RenderAttachment = 0x10
};

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUsage usage = TextureUsage::None;
};

struct Texture : public Lattice::Component {
    using Desc = TextureDesc;
    virtual ~Texture() = default;
};

}