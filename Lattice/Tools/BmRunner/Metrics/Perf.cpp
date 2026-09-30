#include "Perf.hpp"

#include <cerrno>
#include <cstring>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <Lattice/Tools/Exception.hpp>

namespace Lattice::Benchmarks {

namespace {

int perfEventOpen(perf_event_attr& attr, int group) {
    return static_cast<int>(::syscall(
        SYS_perf_event_open,
        &attr,
        0,
        -1,
        group,
        0
    ));
}

}

Perf::Perf() {
    leader_ = openCounter(PERF_COUNT_HW_CPU_CYCLES, -1);

    if (leader_ < 0)
        return;

    fds_[0] = leader_;
    fds_[1] = openCounter(PERF_COUNT_HW_INSTRUCTIONS, leader_);
    fds_[2] = openCounter(PERF_COUNT_HW_CACHE_REFERENCES, leader_);
    fds_[3] = openCounter(PERF_COUNT_HW_CACHE_MISSES, leader_);
    fds_[4] = openCounter(PERF_COUNT_HW_BRANCH_INSTRUCTIONS, leader_);
    fds_[5] = openCounter(PERF_COUNT_HW_BRANCH_MISSES, leader_);

    for (int fd : fds_) {
        if (fd < 0) {
            close();
            return;
        }
    }
}

Perf::~Perf() {
    close();
}

int Perf::openCounter(uint64_t config, int group) {
    perf_event_attr attr{};

    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = group < 0 ? 1 : 0;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format =
        PERF_FORMAT_GROUP |
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING;

    return perfEventOpen(attr, group);
}

void Perf::start() {
    if (!available())
        return;

    if (
        ::ioctl(
            leader_,
            PERF_EVENT_IOC_RESET,
            PERF_IOC_FLAG_GROUP
        ) == -1
    ) {
        throw Exception<Perf>(
            "Failed to reset counters: {}",
            std::strerror(errno)
        );
    }

    if (
        ::ioctl(
            leader_,
            PERF_EVENT_IOC_ENABLE,
            PERF_IOC_FLAG_GROUP
        ) == -1
    ) {
        throw Exception<Perf>(
            "Failed to enable counters: {}",
            std::strerror(errno)
        );
    }
}

Perf::Result Perf::stop() {
    if (!available())
        return {};

    if (
        ::ioctl(
            leader_,
            PERF_EVENT_IOC_DISABLE,
            PERF_IOC_FLAG_GROUP
        ) == -1
    ) {
        throw Exception<Perf>(
            "Failed to disable counters: {}",
            std::strerror(errno)
        );
    }

    struct ReadData {
        uint64_t count;
        uint64_t timeEnabled;
        uint64_t timeRunning;
        uint64_t values[CounterCount];
    } data{};

    const ssize_t bytes =
        ::read(
            leader_,
            &data,
            sizeof(data)
        );

    if (bytes != static_cast<ssize_t>(sizeof(data))) {
        throw Exception<Perf>(
            "Failed to read counters: {}",
            std::strerror(errno)
        );
    }

    if (data.count != CounterCount) {
        throw Exception<Perf>(
            "Expected {} counters, got {}",
            CounterCount,
            data.count
        );
    }

    auto value = [&](size_t index) -> uint64_t {
        if (data.timeRunning == 0)
            return 0;

        if (data.timeRunning == data.timeEnabled)
            return data.values[index];

        const long double scaled =
            static_cast<long double>(data.values[index]) *
            static_cast<long double>(data.timeEnabled) /
            static_cast<long double>(data.timeRunning);

        return static_cast<uint64_t>(scaled);
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
    start();
}

Metrics Perf::end() {
    if (!available())
        return {};

    const Result value = stop();

    totals_ += value;
    ++samples_;

    return makeMetrics(value);
}

Metrics Perf::finish() {
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

Metrics Perf::makeMetrics(
    const Result& value
) {
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
    for (int& fd : fds_) {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    leader_ = -1;
}

}