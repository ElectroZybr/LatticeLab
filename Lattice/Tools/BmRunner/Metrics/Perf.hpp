#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Analysis.hpp>

namespace Lattice::Benchmarks {

class Perf : public Capability {
public:
    enum Metric : uint8_t {
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
        MPKI,
        _count
    };

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

    static constexpr MetricDesc schema_[] = {
        {"cycles", Unit::Cycles, MetricFlags::PerIteration},
        {"instructions", Unit::Count, MetricFlags::PerIteration},
        {"cacheReferences", Unit::Count, MetricFlags::PerIteration},
        {"cacheMisses", Unit::Count, MetricFlags::PerIteration},
        {"branches", Unit::Count, MetricFlags::PerIteration},
        {"branchMisses", Unit::Count, MetricFlags::PerIteration},
        {"IPC", Unit::Ratio, MetricFlags::Live},
        {"CPI", Unit::Ratio},
        {"cacheMissRate", Unit::Percent, MetricFlags::Live},
        {"branchMissRate", Unit::Percent, MetricFlags::Live},
        {"IPB", Unit::Ratio},
        {"MPKI", Unit::Ratio, MetricFlags::Live}
    };

    int leader_ = -1;
    std::array<int, CounterCount> fds_{-1, -1, -1, -1, -1, -1};

    Result sample_{};
    Result totals_{};
    size_t samples_ = 0;

public:
    Perf();
    ~Perf();

    Perf(const Perf&) = delete;
    Perf& operator=(const Perf&) = delete;

    std::string_view name() const noexcept override {
        return "Perf";
    }

    bool available() const noexcept {
        return leader_ >= 0;
    }

    void begin() override;
    void start() override;
    void stop() override;
    Metrics end() override;
    Metrics result() override;

private:
    static Metrics makeMetrics(const Result& value);

    Result readCounters();
    int openCounter(uint64_t config, int group);
    void close();
};

constexpr ValueRef valueRef(Perf::Metric metric) {
    return {
        .source = ValueSource::Metric,
        .capability = "Perf",
        .index = static_cast<size_t>(metric)
    };
}

}