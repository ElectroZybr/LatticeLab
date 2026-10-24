#include "Timer.hpp"

#include <cstring>

#include <Lattice/Tools/Exception.hpp>

#include "Native.hpp"

namespace Wgpu {

using Error = Lattice::Exception<Timer>;

Timer::Timer(Device& device, uint32_t capacity, std::string label)
    : Tracked(device, "Timer", label, uint64_t(capacity) * 16), device_(device), capacity_(capacity) {
    if (!capacity)
        throw Error("timer '{}' needs room for at least one interval", label);
    if (!device.has(Feature::Timestamps))
        return;
    WGPUQuerySetDescriptor desc{};
    desc.label = nativeString(label);
    desc.type = WGPUQueryType_Timestamp;
    desc.count = capacity * 2;
    querySet_ = wgpuDeviceCreateQuerySet(device.native(), &desc);
    if (!querySet_)
        throw Error("failed to create the query set of timer '{}'", label);
    resolved_ = std::make_unique<Buffer>(device, BufferDesc{.size = uint64_t(capacity) * 16,
        .usage = BufferUsage::QueryResolve | BufferUsage::CopySource, .label = label + "/resolved"});
    owners_.reserve(capacity);
    // Stable addresses: a pass keeps the pointer until it begins.
    writes_.reserve(capacity);
    renderWrites_.reserve(capacity);
}

Timer::~Timer() {
    // Release only: in wgpu-native Destroy already frees the query set.
    if (querySet_)
        wgpuQuerySetRelease(querySet_);
}

std::string_view Timer::unavailableReason() const noexcept {
    return available() ? std::string_view{} : "the device has no timestamp queries";
}

uint32_t Timer::take(std::string_view name) {
    uint32_t owner = 0;
    while (owner < intervals_.size() && intervals_[owner].name != name)
        ++owner;
    if (owner == intervals_.size())
        intervals_.push_back({.name = std::string(name)});
    if (!available() || owners_.size() == capacity_) {
        ++intervals_[owner].skipped;
        return NoInterval;
    }
    owners_.push_back(owner);
    return uint32_t(owners_.size() - 1);
}

const WGPUComputePassTimestampWrites* Timer::pass(std::string_view name) {
    const uint32_t interval = take(name);
    if (interval == NoInterval)
        return nullptr;
    writes_.push_back({.querySet = querySet_, .beginningOfPassWriteIndex = interval * 2,
                       .endOfPassWriteIndex = interval * 2 + 1});
    return &writes_.back();
}

const WGPURenderPassTimestampWrites* Timer::renderPass(std::string_view name) {
    const uint32_t interval = take(name);
    if (interval == NoInterval)
        return nullptr;
    renderWrites_.push_back({.querySet = querySet_, .beginningOfPassWriteIndex = interval * 2,
                             .endOfPassWriteIndex = interval * 2 + 1});
    return &renderWrites_.back();
}

uint32_t Timer::begin(CommandList& commands, std::string_view name) {
    if (!device_.has(Feature::EncoderTimestamps)) {
        take(name);   // counted as skipped when unavailable
        return NoInterval;
    }
    const uint32_t interval = take(name);
    if (interval != NoInterval)
        wgpuCommandEncoderWriteTimestamp(commands.encoder(), querySet_, interval * 2);
    return interval;
}

void Timer::end(CommandList& commands, uint32_t interval) {
    if (interval == NoInterval)
        return;
    if (interval >= owners_.size())
        throw Error("end of an interval {} that was not begun", interval);
    wgpuCommandEncoderWriteTimestamp(commands.encoder(), querySet_, interval * 2 + 1);
}

std::vector<Timer::Interval> Timer::read() {
    const auto used = uint32_t(owners_.size());
    if (used) {
        // Resolving needs only that the writing work was submitted: a separate list.
        CommandList commands{device_};
        wgpuCommandEncoderResolveQuerySet(commands.encoder(), querySet_, 0, used * 2, resolved_->native(), 0);
        commands.submit();
        const auto bytes = device_.readBuffer(*resolved_, 0, uint64_t(used) * 16);
        const double period = device_.timestampPeriod();
        for (uint32_t i = 0; i < used; ++i) {
            uint64_t ticks[2];
            std::memcpy(ticks, bytes.data() + i * 16, 16);
            // Timestamps of one pass can come out of order on some drivers: no negatives.
            const double nanoseconds = ticks[1] > ticks[0] ? double(ticks[1] - ticks[0]) * period : 0.0;
            auto& interval = intervals_[owners_[i]];
            interval.nanoseconds += nanoseconds;
            ++interval.count;
        }
    }
    std::vector<Interval> result = intervals_;
    for (auto& interval : intervals_)
        interval = {.name = std::move(interval.name)};
    owners_.clear();
    writes_.clear();
    renderWrites_.clear();
    return result;
}

}
