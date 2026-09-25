#pragma once

#include <cstddef>
#include <cstdint>

#include <Lattice/Kernel/Consts.hpp>

namespace GPU {

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
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has(BufferUsage value, BufferUsage flag) {
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

struct BufferDesc {
    std::size_t size = 0;
    BufferUsage usage = BufferUsage::None;
};

struct Buffer : public Lattice::Component {
    using Desc = BufferDesc;
    virtual ~Buffer() = default;
};

}