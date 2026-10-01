#include "AmdDfMemoryTraffic.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

#include <linux/perf_event.h>

namespace Lattice::Benchmarks {

namespace {

static constexpr uint64_t configs[] = {
    0x0000000000003807,
    0x0000000000003847,
    0x0000000000003887,
    0x00000000000038c7,
    0x0000000100003807,
    0x0000000100003847,
    0x0000000100003887,
    0x00000001000038c7
};

int readInt(const char* path) {
    std::ifstream file(path);

    int value = -1;
    file >> value;

    return value;
}

int readCpu() {
    std::ifstream file(
        "/sys/bus/event_source/devices/amd_df/cpumask"
    );

    std::string mask;
    file >> mask;

    if (mask.empty())
        return -1;

    try {
        return std::stoi(mask);
    } catch (...) {
        return -1;
    }
}

}

AmdDfMemoryTraffic::AmdDfMemoryTraffic() {
    const int type = readInt(
        "/sys/bus/event_source/devices/amd_df/type"
    );

    const int cpu = readCpu();

    if (type < 0 || cpu < 0) {
        unavailableReason_ =
            "amd_df PMU is missing; run 'sudo modprobe amd_uncore' "
            "and add amd_uncore to /etc/modules-load.d/amd-uncore.conf";
        return;
    }

    for (size_t i = 0; i < CounterCount; ++i) {
        events_[i] = openCounter(
            static_cast<uint32_t>(type),
            cpu,
            configs[i]
        );

        if (!events_[i].available()) {
            unavailableReason_ = std::string(
                "perf_event_open failed: "
            ) + std::strerror(errno) +
                "; try 'sudo sysctl -w kernel.perf_event_paranoid=-1'";
            close();
            return;
        }
    }
}

bool AmdDfMemoryTraffic::available() const noexcept {
    for (const PerfEvent& event : events_)
        if (!event.available())
            return false;

    return true;
}

std::string_view AmdDfMemoryTraffic::unavailableReason() const noexcept {
    return unavailableReason_;
}

void AmdDfMemoryTraffic::begin() {
    if (!available())
        return;

    for (PerfEvent& event : events_)
        event.reset();

    for (size_t i = 0; i < CounterCount; ++i)
        starts_[i] = events_[i].snapshot();

    for (PerfEvent& event : events_)
        event.enable();
}

uint64_t AmdDfMemoryTraffic::end() {
    if (!available())
        return 0;

    uint64_t count = 0;

    for (size_t i = 0; i < CounterCount; ++i) {
        count += PerfEvent::delta(
            starts_[i],
            events_[i].snapshot()
        );
    }

    for (PerfEvent& event : events_)
        event.disable();

    constexpr uint64_t BytesPerCount = 64;

    return count * BytesPerCount;
}

PerfEvent AmdDfMemoryTraffic::openCounter(
    uint32_t type,
    int cpu,
    uint64_t config
) {
    perf_event_attr attr{};

    attr.type = type;
    attr.config = config;
    attr.disabled = 1;

    attr.read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING;

    return PerfEvent(attr, -1, cpu);
}

void AmdDfMemoryTraffic::close() {
    for (PerfEvent& event : events_)
        event = {};
}

}
