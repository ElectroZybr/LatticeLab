#pragma once

#include <cstdint>

#include <linux/perf_event.h>

namespace Lattice::Benchmarks {

class PerfEvent {
public:
    struct Snapshot {
        uint64_t value = 0;
        uint64_t timeEnabled = 0;
        uint64_t timeRunning = 0;
    };

private:
    int fd_ = -1;
    perf_event_mmap_page* page_ = nullptr;

public:
    PerfEvent() = default;

    PerfEvent(
        perf_event_attr attr,
        int pid,
        int cpu,
        int group = -1,
        unsigned long flags = 0
    );

    ~PerfEvent();

    PerfEvent(const PerfEvent&) = delete;
    PerfEvent& operator=(const PerfEvent&) = delete;

    PerfEvent(PerfEvent&& other) noexcept;
    PerfEvent& operator=(PerfEvent&& other) noexcept;

    bool available() const noexcept {
        return fd_ >= 0;
    }

    int fd() const noexcept {
        return fd_;
    }

    bool fastReadable() const noexcept;

    void reset(unsigned long flags = 0);
    void enable(unsigned long flags = 0);
    void disable(unsigned long flags = 0);

    // Uses the perf mmap page and RDPMC when the kernel exposes it.
    // Falls back to read(2) on unsupported PMUs/architectures.
    Snapshot snapshot() const;

    // Returns the interval count scaled by time_enabled/time_running.
    static uint64_t delta(
        const Snapshot& begin,
        const Snapshot& end
    );

private:
    Snapshot snapshotRdpmc() const noexcept;
    Snapshot snapshotRead() const;

    void map();
    void close();
};

}
