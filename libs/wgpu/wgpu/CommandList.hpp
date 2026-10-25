#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

#include "Resources.hpp"

namespace Wgpu {

/// One batch of recorded commands (a frame, a copy, a compute step); submitted once.
/// Not tracked as a resource. Passes (render, compute) record into its encoder and
/// must end before submit.
class CommandList final {
public:
    explicit CommandList(Device& device);
    ~CommandList();
    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    /// Offsets and size must be multiples of 4; buffers need CopySource / CopyDestination.
    void copyBuffer(Buffer& source, uint64_t sourceOffset, Buffer& target, uint64_t targetOffset, uint64_t size);
    /// Mip 0 of a 2D texture (CopySource) into a buffer (CopyDestination), row after row
    /// `bytesPerRow` apart — a multiple of 256 (RowAlignment), at least a row of texels.
    void copyTexture(Texture& source, Buffer& target, uint32_t bytesPerRow);
    static constexpr uint32_t RowAlignment = 256;
    void submit();

    Device& device() const noexcept { return device_; }

    /// For passes: the encoder to record into, and the open pass count submit checks.
    WGPUCommandEncoder encoder();
    void passBegan() noexcept { ++openPasses_; }
    void passEnded() noexcept { --openPasses_; }

private:
    void requireRecording() const;

    Device& device_;
    WGPUCommandEncoder encoder_ = nullptr;
    WGPUCommandBuffer commandBuffer_ = nullptr;
    int openPasses_ = 0;
    bool submitted_ = false;
};

}
