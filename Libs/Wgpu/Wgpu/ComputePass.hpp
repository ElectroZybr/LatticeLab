#pragma once

#include <cstdint>
#include <string_view>

#include <webgpu/webgpu.h>

#include "CommandList.hpp"
#include "Resources.hpp"

namespace Wgpu {

class Timer;

/// A compute pass recorded into a command list. Begins on construction, ends with
/// end() or on destruction; the list submits after it ends. With a timer, the GPU time
/// of the pass is measured as an interval named by the label.
class ComputePass final {
public:
    explicit ComputePass(CommandList& commands, std::string_view label = {}, Timer* timer = nullptr);
    ~ComputePass();
    ComputePass(const ComputePass&) = delete;
    ComputePass& operator=(const ComputePass&) = delete;

    void setPipeline(ComputePipeline& pipeline);
    void setBindings(uint32_t group, BindingSet& bindings);
    /// Workgroup counts per dimension (not invocations).
    void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1);
    void end();

private:
    void requireActive() const;

    CommandList& commands_;
    WGPUComputePassEncoder pass_ = nullptr;
    bool pipeline_ = false;
};

/// Workgroups to cover `count` invocations with groups of `size`.
constexpr uint32_t workgroups(uint64_t count, uint32_t size) noexcept {
    return uint32_t((count + size - 1) / size);
}

}
