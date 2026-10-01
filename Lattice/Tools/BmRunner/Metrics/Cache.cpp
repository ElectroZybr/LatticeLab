#include "Cache.hpp"

#include <cstring>

#include <linux/perf_event.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

namespace Lattice::Benchmarks {

namespace {

constexpr uint64_t rawConfig(uint64_t event, uint64_t mask) noexcept {
    return event | (mask << 8);
}

// AMD Zen core PMU proxies for per-thread data LLC traffic:
// - L2 misses caused by L1D misses become L3 accesses;
// - L1D fills sourced from local/remote memory or IO missed the L3.
constexpr uint64_t AmdZenLlcReads = rawConfig(0x64, 0x08);
constexpr uint64_t AmdZenLlcMisses = rawConfig(0x44, 0x48);

}

Cache::Result& Cache::Result::operator+=(const Result& other) noexcept {
    instructions += other.instructions;
    l1dReads += other.l1dReads;
    l1dMisses += other.l1dMisses;
    llcReads += other.llcReads;
    llcMisses += other.llcMisses;
    dtlbMisses += other.dtlbMisses;
    return *this;
}

double Cache::Result::l1dMissRate() const noexcept {
    return l1dReads
        ? static_cast<double>(l1dMisses) / static_cast<double>(l1dReads)
        : 0.0;
}

double Cache::Result::llcMissRate() const noexcept {
    return llcReads
        ? static_cast<double>(llcMisses) / static_cast<double>(llcReads)
        : 0.0;
}

double Cache::Result::mpki(uint64_t misses) const noexcept {
    return Cache::missesPerKiloInstructions(misses, instructions);
}

Cache::Cache() {
    events_[Instructions] = openEvent(
        PERF_TYPE_HARDWARE,
        PERF_COUNT_HW_INSTRUCTIONS
    );

    events_[L1dReads] = openCacheEvent(
        PERF_COUNT_HW_CACHE_L1D,
        PERF_COUNT_HW_CACHE_OP_READ,
        PERF_COUNT_HW_CACHE_RESULT_ACCESS
    );
    events_[L1dMisses] = openCacheEvent(
        PERF_COUNT_HW_CACHE_L1D,
        PERF_COUNT_HW_CACHE_OP_READ,
        PERF_COUNT_HW_CACHE_RESULT_MISS
    );
    events_[LlcReads] = openCacheEvent(
        PERF_COUNT_HW_CACHE_LL,
        PERF_COUNT_HW_CACHE_OP_READ,
        PERF_COUNT_HW_CACHE_RESULT_ACCESS
    );
    events_[LlcMisses] = openCacheEvent(
        PERF_COUNT_HW_CACHE_LL,
        PERF_COUNT_HW_CACHE_OP_READ,
        PERF_COUNT_HW_CACHE_RESULT_MISS
    );
    events_[DtlbMisses] = openCacheEvent(
        PERF_COUNT_HW_CACHE_DTLB,
        PERF_COUNT_HW_CACHE_OP_READ,
        PERF_COUNT_HW_CACHE_RESULT_MISS
    );

    if (
        (!events_[LlcReads].available() || !events_[LlcMisses].available()) &&
        supportsAmdZenRawEvents()
    ) {
        events_[LlcReads] = openEvent(
            PERF_TYPE_RAW,
            AmdZenLlcReads
        );
        events_[LlcMisses] = openEvent(
            PERF_TYPE_RAW,
            AmdZenLlcMisses
        );
    }

    if (!available())
        close();
}

bool Cache::available() const noexcept {
    for (const PerfEvent& event : events_)
        if (!event.available())
            return false;

    return true;
}

void Cache::begin() {
    sample_ = {};

    if (!available())
        return;

    for (PerfEvent& event : events_) {
        event.reset();
        event.enable();
    }
}

void Cache::start() {
    if (!available())
        return;

    for (size_t i = 0; i < CounterCount; ++i)
        starts_[i] = events_[i].snapshot();
}

void Cache::stop() {
    if (!available())
        return;

    sample_ += readCounters();
}

Metrics Cache::end() {
    if (!available())
        return {};

    for (PerfEvent& event : events_)
        event.disable();

    totals_ += sample_;
    ++samples_;

    return makeMetrics(sample_);
}

Metrics Cache::result() {
    if (samples_ == 0)
        return {};

    const double samples = static_cast<double>(samples_);
    Metrics metrics = makeMetrics(totals_);

    for (size_t i = 0; i < metrics.values.size(); ++i)
        if (hasFlag(metrics.schema[i].flags, MetricFlags::PerIteration))
            metrics.values[i] /= samples;

    totals_ = {};
    samples_ = 0;

    return metrics;
}

Cache::Result Cache::readCounters() const {
    auto value = [&](size_t index) {
        return PerfEvent::delta(
            starts_[index],
            events_[index].snapshot()
        );
    };

    return {
        .instructions = value(Instructions),
        .l1dReads = value(L1dReads),
        .l1dMisses = value(L1dMisses),
        .llcReads = value(LlcReads),
        .llcMisses = value(LlcMisses),
        .dtlbMisses = value(DtlbMisses)
    };
}

Metrics Cache::makeMetrics(const Result& result) {
    return {
        .schema = schema(),
        .values = {
            static_cast<double>(result.l1dReads),
            static_cast<double>(result.l1dMisses),
            result.l1dMissRate() * 100.0,
            result.mpki(result.l1dMisses),
            static_cast<double>(result.llcReads),
            static_cast<double>(result.llcMisses),
            result.llcMissRate() * 100.0,
            result.mpki(result.llcMisses),
            static_cast<double>(result.dtlbMisses),
            result.mpki(result.dtlbMisses)
        }
    };
}

uint64_t Cache::cacheConfig(
    uint64_t cache,
    uint64_t operation,
    uint64_t result
) noexcept {
    return cache | (operation << 8) | (result << 16);
}

PerfEvent Cache::openEvent(uint32_t type, uint64_t config) {
    perf_event_attr attr{};

    attr.type = type;
    attr.config = config;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING;

    return PerfEvent(attr, 0, -1);
}

PerfEvent Cache::openCacheEvent(
    uint64_t cache,
    uint64_t operation,
    uint64_t result
) {
    return openEvent(
        PERF_TYPE_HW_CACHE,
        cacheConfig(cache, operation, result)
    );
}

bool Cache::supportsAmdZenRawEvents() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;

    if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx))
        return false;

    char vendor[13]{};
    std::memcpy(vendor, &ebx, sizeof(ebx));
    std::memcpy(vendor + 4, &edx, sizeof(edx));
    std::memcpy(vendor + 8, &ecx, sizeof(ecx));

    if (std::string_view(vendor) != "AuthenticAMD")
        return false;

    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return false;

    const unsigned int baseFamily = (eax >> 8) & 0x0f;
    const unsigned int extendedFamily = (eax >> 20) & 0xff;
    const unsigned int family =
        baseFamily == 0x0f
            ? baseFamily + extendedFamily
            : baseFamily;

    return family >= 0x17 && family <= 0x1a;
#else
    return false;
#endif
}

void Cache::close() {
    for (PerfEvent& event : events_)
        event = {};
}

}
