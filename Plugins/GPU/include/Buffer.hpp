#pragma once

#include <cstddef>
#include <cstdint>
#include "Lattice/Tools/Logger.hpp"

namespace Lattice {
class Node;
}

namespace GPU {

enum class BufferUsage : uint32_t {
    None            = 0,
    Vertex          = 1 << 0,
    Index           = 1 << 1,
    Uniform         = 1 << 2,
    Storage         = 1 << 3,
    CopySource      = 1 << 4,
    CopyDestination = 1 << 5
};

struct BufferDesc {
    std::size_t size;
    BufferUsage usage;
};

struct Buffer {
    virtual ~Buffer() = default;
};

// struct BufferImpl : public Buffer {
//     using Desc = BufferDesc;
//     BufferImpl(Lattice::Node& br, const BufferDesc& desc) {
//         Logger::ok("BufferImpl", "created!!! desc size {}", desc.size);
//     }
// };

}