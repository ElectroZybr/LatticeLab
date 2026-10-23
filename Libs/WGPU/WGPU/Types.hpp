#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace WGPU {

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

struct PipelineDesc {
    TextureFormat colorFormat = TextureFormat::BGRA8Unorm;
    std::string vertexEntry = "vs";
    std::string fragmentEntry = "fs";
    std::string label;
};

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

struct Binding {
    uint32_t binding = 0;
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
};

/// Native window handles a surface presents to. Produced by the window layer.
struct NativeWindow {
    enum class Kind { None, Win32, X11, Wayland, Cocoa, Metal, Web, Headless };
    Kind kind = Kind::None;
    void* display = nullptr;
    void* window = nullptr;
    void* extra = nullptr;
    std::shared_ptr<void> owner;
    bool operator==(const NativeWindow&) const = default;
};

}
