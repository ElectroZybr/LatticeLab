#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <webgpu/webgpu.h>

#include "CommandList.hpp"
#include "Device.hpp"
#include "Resources.hpp"

namespace Wgpu {

/// GPU time of passes and copies: each interval writes two timestamps into a query set;
/// read() resolves every interval since the last read and sums them by name. Reading
/// waits for the GPU, so measuring code reads after the timed work (once per sample).
/// Without Feature::Timestamps the timer is unavailable and records nothing.
class Timer final : Tracked {
public:
    explicit Timer(Device& device, uint32_t capacity = 1024, std::string label = "timer");
    ~Timer();

    bool available() const noexcept { return querySet_ != nullptr; }
    std::string_view unavailableReason() const noexcept;

    /// Timestamp writes of a compute pass measured as `name` (ComputePass takes the
    /// timer); nullptr when unavailable or full — the pass then runs untimed.
    const WGPUComputePassTimestampWrites* pass(std::string_view name);
    /// The same for a render pass (the renderer passes them to its pass).
    const WGPURenderPassTimestampWrites* renderPass(std::string_view name);

    /// An interval between commands of an encoder (copies); needs EncoderTimestamps.
    /// begin() returns an interval id, or NoInterval when it cannot be measured.
    static constexpr uint32_t NoInterval = ~0u;
    uint32_t begin(CommandList& commands, std::string_view name);
    void end(CommandList& commands, uint32_t interval);

    struct Interval {
        std::string name;
        double nanoseconds = 0;   // sum over the measured intervals
        uint64_t count = 0;       // measured intervals
        uint64_t skipped = 0;     // not measured: the timer was full
    };
    /// Every interval since the last read, by name; waits for the GPU.
    std::vector<Interval> read();

private:
    uint32_t take(std::string_view name);

    Device& device_;
    uint32_t capacity_;
    WGPUQuerySet querySet_ = nullptr;
    std::unique_ptr<Buffer> resolved_;
    std::vector<uint32_t> owners_;          // name of each used interval
    std::vector<Interval> intervals_;       // by name, in order of first use
    std::vector<WGPUComputePassTimestampWrites> writes_;
    std::vector<WGPURenderPassTimestampWrites> renderWrites_;
};

}
