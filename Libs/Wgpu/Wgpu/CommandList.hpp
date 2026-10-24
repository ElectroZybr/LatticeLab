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
