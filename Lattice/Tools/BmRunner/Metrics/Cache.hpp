#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <Lattice/Tools/BmRunner/Metrics/PerfEvent.hpp>
#include <Lattice/Tools/BmRunner/Stages.hpp>

namespace Lattice::Benchmarks {

class Cache : public MetricCapability<"Cache", Cache> {
public:
    inline static constexpr auto l1dReads = defineMetric(
        "l1dReads",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );
    inline static constexpr auto l1dMisses = defineMetric(
        "l1dMisses",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );
    inline static constexpr auto l1dMissRate = defineMetric(
        "l1dMissRate", Unit::Percent, MetricFlags::Live
    );
    inline static constexpr auto l1dMPKI = defineMetric(
        "l1dMPKI", Unit::Ratio, MetricFlags::Live
    );
    inline static constexpr auto llcReads = defineMetric(
        "llcReads",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );
    inline static constexpr auto llcMisses = defineMetric(
        "llcMisses",
        Unit::Count,
        MetricFlags::PerIteration | MetricFlags::Live
    );
    inline static constexpr auto llcMissRate = defineMetric(
        "llcMissRate", Unit::Percent
    );
    inline static constexpr auto llcMPKI = defineMetric(
        "llcMPKI", Unit::Ratio, MetricFlags::Live
    );
    inline static constexpr auto dtlbMisses = defineMetric(
        "dtlbMisses", Unit::Count, MetricFlags::PerIteration
    );
    inline static constexpr auto dtlbMPKI = defineMetric(
        "dtlbMPKI", Unit::Ratio, MetricFlags::Live
    );

    inline static constexpr auto Schema = defineSchema(
        l1dReads,
        l1dMisses,
        l1dMissRate,
        l1dMPKI,
        llcReads,
        llcMisses,
        llcMissRate,
        llcMPKI,
        dtlbMisses,
        dtlbMPKI
    );

private:
    static constexpr size_t CounterCount = 6;

    enum Counter : size_t {
        Instructions,
        L1dReads,
        L1dMisses,
        LlcReads,
        LlcMisses,
        DtlbMisses
    };

    struct Result {
        uint64_t instructions = 0;
        uint64_t l1dReads = 0;
        uint64_t l1dMisses = 0;
        uint64_t llcReads = 0;
        uint64_t llcMisses = 0;
        uint64_t dtlbMisses = 0;

        Result& operator+=(const Result& other) noexcept;

        double l1dMissRate() const noexcept;
        double llcMissRate() const noexcept;
        double mpki(uint64_t misses) const noexcept;
    };

    std::array<PerfEvent, CounterCount> events_;
    std::array<PerfEvent::Snapshot, CounterCount> starts_;

    Result sample_{};
    Result totals_{};
    size_t samples_ = 0;

public:
    static constexpr double missesPerKiloInstructions(
        uint64_t misses,
        uint64_t instructions
    ) noexcept {
        return instructions
            ? static_cast<double>(misses) * 1000.0 /
              static_cast<double>(instructions)
            : 0.0;
    }

    Cache();
    ~Cache() override = default;

    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    bool available() const noexcept override;

    std::string_view unavailableReason() const noexcept override {
        return
            "required cache perf events are unsupported or unavailable; try "
            "'sudo sysctl -w kernel.perf_event_paranoid=-1'";
    }

    void begin() override;
    void start() override;
    void stop() override;

    Metrics end() override;
    Metrics result() override;

private:
    Result readCounters() const;
    static Metrics makeMetrics(const Result& result);

    static uint64_t cacheConfig(
        uint64_t cache,
        uint64_t operation,
        uint64_t result
    ) noexcept;

    static PerfEvent openEvent(uint32_t type, uint64_t config);
    static PerfEvent openCacheEvent(
        uint64_t cache,
        uint64_t operation,
        uint64_t result
    );

    static bool supportsAmdZenRawEvents() noexcept;
    void close();
};

}
