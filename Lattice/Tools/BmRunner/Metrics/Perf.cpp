#include "Perf.hpp"

#include <linux/perf_event.h>

namespace Lattice::Benchmarks {

Perf::Perf() {
    events_[0] = openCounter(
        PERF_COUNT_HW_CPU_CYCLES,
        -1
    );

    if (!events_[0].available())
        return;

    const int leader = events_[0].fd();

    events_[1] = openCounter(PERF_COUNT_HW_INSTRUCTIONS, leader);
    events_[2] = openCounter(PERF_COUNT_HW_CACHE_REFERENCES, leader);
    events_[3] = openCounter(PERF_COUNT_HW_CACHE_MISSES, leader);
    events_[4] = openCounter(PERF_COUNT_HW_BRANCH_INSTRUCTIONS, leader);
    events_[5] = openCounter(PERF_COUNT_HW_BRANCH_MISSES, leader);

    for (const PerfEvent& event : events_) {
        if (!event.available()) {
            close();
            return;
        }
    }
}

PerfEvent Perf::openCounter(uint64_t config, int group) {
    perf_event_attr attr{};

    attr.type = PERF_TYPE_HARDWARE;
    attr.config = config;
    attr.disabled = group < 0 ? 1 : 0;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING;

    return PerfEvent(attr, 0, -1, group);
}

Perf::Result Perf::readCounters() const {
    if (!available())
        return {};

    auto value = [&](size_t index) {
        return PerfEvent::delta(
            starts_[index],
            events_[index].snapshot()
        );
    };

    return {
        .cycles = value(0),
        .instructions = value(1),
        .cacheReferences = value(2),
        .cacheMisses = value(3),
        .branches = value(4),
        .branchMisses = value(5)
    };
}

void Perf::begin() {
    sample_ = {};

    if (!available())
        return;

    events_[0].reset(PERF_IOC_FLAG_GROUP);
    events_[0].enable(PERF_IOC_FLAG_GROUP);
}

void Perf::start() {
    if (!available())
        return;

    for (size_t i = 0; i < CounterCount; ++i)
        starts_[i] = events_[i].snapshot();
}

void Perf::stop() {
    if (!available())
        return;

    sample_ += readCounters();
}

Metrics Perf::end() {
    if (!available())
        return {};

    events_[0].disable(PERF_IOC_FLAG_GROUP);

    totals_ += sample_;
    ++samples_;

    return makeMetrics(sample_);
}

Metrics Perf::result() {
    if (samples_ == 0)
        return {};

    const double samples =
        static_cast<double>(samples_);

    Metrics metrics{
        .schema = schema_,
        .values = {
            static_cast<double>(totals_.cycles) / samples,
            static_cast<double>(totals_.instructions) / samples,
            static_cast<double>(totals_.cacheReferences) / samples,
            static_cast<double>(totals_.cacheMisses) / samples,
            static_cast<double>(totals_.branches) / samples,
            static_cast<double>(totals_.branchMisses) / samples,
            totals_.ipc(),
            totals_.cpi(),
            totals_.cacheMissRate() * 100.0,
            totals_.branchMissRate() * 100.0,
            totals_.instructionsPerBranch(),
            totals_.mpki()
        }
    };

    totals_ = {};
    samples_ = 0;

    return metrics;
}

Metrics Perf::makeMetrics(const Result& value) {
    return {
        .schema = schema_,
        .values = {
            static_cast<double>(value.cycles),
            static_cast<double>(value.instructions),
            static_cast<double>(value.cacheReferences),
            static_cast<double>(value.cacheMisses),
            static_cast<double>(value.branches),
            static_cast<double>(value.branchMisses),
            value.ipc(),
            value.cpi(),
            value.cacheMissRate() * 100.0,
            value.branchMissRate() * 100.0,
            value.instructionsPerBranch(),
            value.mpki()
        }
    };
}

void Perf::close() {
    for (PerfEvent& event : events_)
        event = {};
}

}
