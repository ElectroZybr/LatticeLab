#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace Wgpu {

class Buffer;

enum class BufferUsage : uint64_t {
    None            = 0,
    CopySource      = 1ull << 2,
    CopyDestination = 1ull << 3,
    Index           = 1ull << 4,
    Vertex          = 1ull << 5,
    Uniform         = 1ull << 6,
    Storage         = 1ull << 7
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

constexpr bool has(BufferUsage value, BufferUsage flag) {
    return (static_cast<uint64_t>(value) & static_cast<uint64_t>(flag)) != 0;
}

struct BufferDesc {
    std::size_t size = 0;
    BufferUsage usage = BufferUsage::None;
    std::string label;
};

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

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUsage usage = TextureUsage::None;
    std::string label;
};

struct ShaderDesc {
    std::string source;   // WGSL
    std::string label;
};

enum class ShaderStage : uint32_t {
    None     = 0,
    Vertex   = 1u << 0,
    Fragment = 1u << 1,
    Compute  = 1u << 2
};

constexpr ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class BindingType {
    Uniform,
    Storage,
    ReadOnlyStorage
};

/// One buffer binding of a bind group layout.
struct LayoutEntry {
    uint32_t binding = 0;
    BindingType type = BindingType::Uniform;
    ShaderStage visibility = ShaderStage::Vertex | ShaderStage::Fragment;
};

struct Binding {
    uint32_t binding = 0;
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
};

}
