#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Metrics/PerfEvent.hpp>

namespace Lattice::Benchmarks {

class Perf : public MetricCapability<"Perf", Perf> {
public:
    inline static constexpr auto cycles = defineMetric(
        "cycles", Unit::Cycles, MetricFlags::PerIteration
    );
    inline static constexpr auto instructions = defineMetric(
        "instructions", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto cacheReferences = defineMetric(
        "cacheReferences", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto cacheMisses = defineMetric(
        "cacheMisses", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto branches = defineMetric(
        "branches", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto branchMisses = defineMetric(
        "branchMisses", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto IPC = defineMetric(
        "IPC", Unit::Ratio, MetricFlags::Live
    );
    inline static constexpr auto CPI = defineMetric("CPI", Unit::Ratio);
    inline static constexpr auto cacheMissRate = defineMetric(
        "cacheMissRate", Unit::Percent, MetricFlags::Live
    );
    inline static constexpr auto branchMissRate = defineMetric(
        "branchMissRate", Unit::Percent, MetricFlags::Live
    );
    inline static constexpr auto IPB = defineMetric("IPB", Unit::Ratio);
    inline static constexpr auto MPKI = defineMetric(
        "MPKI", Unit::Ratio, MetricFlags::Live
    );

    inline static constexpr auto Schema = defineSchema(
        cycles,
        instructions,
        cacheReferences,
        cacheMisses,
        branches,
        branchMisses,
        IPC,
        CPI,
        cacheMissRate,
        branchMissRate,
        IPB,
        MPKI
    );

private:
    static constexpr size_t CounterCount = 6;

    struct Result {
        uint64_t cycles = 0;
        uint64_t instructions = 0;
        uint64_t cacheReferences = 0;
        uint64_t cacheMisses = 0;
        uint64_t branches = 0;
        uint64_t branchMisses = 0;

        double ipc() const noexcept {
            return cycles
                ? static_cast<double>(instructions) / static_cast<double>(cycles)
                : 0.0;
        }

        double cpi() const noexcept {
            return instructions
                ? static_cast<double>(cycles) / static_cast<double>(instructions)
                : 0.0;
        }

        double cacheMissRate() const noexcept {
            return cacheReferences
                ? static_cast<double>(cacheMisses) / static_cast<double>(cacheReferences)
                : 0.0;
        }

        double branchMissRate() const noexcept {
            return branches
                ? static_cast<double>(branchMisses) / static_cast<double>(branches)
                : 0.0;
        }

        double instructionsPerBranch() const noexcept {
            return branches
                ? static_cast<double>(instructions) / static_cast<double>(branches)
                : 0.0;
        }

        double mpki() const noexcept {
            return instructions
                ? static_cast<double>(cacheMisses) * 1000.0 /
                  static_cast<double>(instructions)
                : 0.0;
        }

        Result& operator+=(const Result& other) noexcept {
            cycles += other.cycles;
            instructions += other.instructions;
            cacheReferences += other.cacheReferences;
            cacheMisses += other.cacheMisses;
            branches += other.branches;
            branchMisses += other.branchMisses;
            return *this;
        }
    };

    std::array<PerfEvent, CounterCount> events_;
    std::array<PerfEvent::Snapshot, CounterCount> starts_;

    Result sample_{};
    Result totals_{};
    size_t samples_ = 0;

public:
    Perf();
    ~Perf() override = default;

    Perf(const Perf&) = delete;
    Perf& operator=(const Perf&) = delete;

    bool available() const noexcept override {
        return events_[0].available();
    }

    std::string_view unavailableReason() const noexcept override {
        return
            "perf events are unavailable; try "
            "'sudo sysctl -w kernel.perf_event_paranoid=-1'";
    }

    void begin() override;
    void start() override;
    void stop() override;
    Metrics end() override;
    Metrics result() override;

private:
    static Metrics makeMetrics(const Result& value);

    Result readCounters() const;
    static PerfEvent openCounter(uint64_t config, int group);
    void close();
};

}
