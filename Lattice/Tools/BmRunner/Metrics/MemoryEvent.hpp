#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <linux/perf_event.h>

#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Metrics/PerfEvent.hpp>

namespace Lattice::Benchmarks {

class MemoryEvent : public MetricCapability<"MemoryEvent", MemoryEvent> {
    static constexpr size_t CounterCount = 2;

    std::array<PerfEvent, CounterCount> events_;
    std::array<PerfEvent::Snapshot, CounterCount> starts_;

    uint64_t sampleMinorFaults_ = 0;
    uint64_t sampleMajorFaults_ = 0;

    uint64_t totalMinorFaults_ = 0;
    uint64_t totalMajorFaults_ = 0;

    size_t samples_ = 0;

public:
    inline static constexpr auto minorFaults = defineMetric(
        "minorFaults",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );

    inline static constexpr auto majorFaults = defineMetric(
        "majorFaults",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );

    inline static constexpr auto Schema = defineSchema(
        minorFaults,
        majorFaults
    );

    MemoryEvent()
        : events_{
            openCounter(PERF_COUNT_SW_PAGE_FAULTS_MIN),
            openCounter(PERF_COUNT_SW_PAGE_FAULTS_MAJ)
        } {}

    bool available() const noexcept override {
        return events_[0].available() && events_[1].available();
    }

    std::string_view unavailableReason() const noexcept override {
        return "memory software perf events are unavailable";
    }

    void begin() override {
        sampleMinorFaults_ = 0;
        sampleMajorFaults_ = 0;

        if (!available())
            return;

        for (auto& event : events_) {
            event.reset();
            event.enable();
        }
    }

    void start() override {
        if (!available())
            return;

        for (size_t i = 0; i < CounterCount; ++i)
            starts_[i] = events_[i].snapshot();
    }

    void stop() override {
        if (!available())
            return;

        const auto minor = events_[0].snapshot();
        const auto major = events_[1].snapshot();

        sampleMinorFaults_ += PerfEvent::delta(starts_[0], minor);
        sampleMajorFaults_ += PerfEvent::delta(starts_[1], major);
    }

    Metrics end() override {
        if (!available())
            return {};

        for (auto& event : events_)
            event.disable();

        totalMinorFaults_ += sampleMinorFaults_;
        totalMajorFaults_ += sampleMajorFaults_;
        ++samples_;

        return {
            .schema = schema(),
            .values = {
                static_cast<double>(sampleMinorFaults_),
                static_cast<double>(sampleMajorFaults_)
            }
        };
    }

    Metrics result() override {
        if (samples_ == 0)
            return {};

        const double divisor = static_cast<double>(samples_);

        Metrics metrics{
            .schema = schema(),
            .values = {
                static_cast<double>(totalMinorFaults_) / divisor,
                static_cast<double>(totalMajorFaults_) / divisor
            }
        };

        totalMinorFaults_ = 0;
        totalMajorFaults_ = 0;
        samples_ = 0;

        return metrics;
    }

private:
    static PerfEvent openCounter(uint64_t config) {
        perf_event_attr attr{};
        attr.type = PERF_TYPE_SOFTWARE;
        attr.size = sizeof(attr);
        attr.config = config;
        attr.disabled = 1;
        attr.exclude_kernel = 0;
        attr.exclude_hv = 1;
        attr.read_format =
            PERF_FORMAT_TOTAL_TIME_ENABLED |
            PERF_FORMAT_TOTAL_TIME_RUNNING;

        return PerfEvent(attr, 0, -1);
    }
};

}