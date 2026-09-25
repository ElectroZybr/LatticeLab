#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

#include "Buffer.hpp"

class NodeBuild;

namespace WGPU {

class Buffer final : public GPU::Buffer {
public:
    explicit Buffer(NodeBuild node, const Desc& desc);
    ~Buffer() override;

    WGPUBuffer native() const noexcept;
    uint64_t size() const noexcept;

private:
    WGPUBuffer buffer_ = nullptr;
    uint64_t size_ = 0;
};

}
