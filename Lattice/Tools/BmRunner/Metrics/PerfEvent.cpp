#include "PerfEvent.hpp"

#include <cerrno>
#include <cstring>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <Lattice/Tools/Exception.hpp>

namespace Lattice::Benchmarks {

namespace {

int perfEventOpen(
    perf_event_attr& attr,
    int pid,
    int cpu,
    int group,
    unsigned long flags
) {
    return static_cast<int>(
        ::syscall(
            SYS_perf_event_open,
            &attr,
            pid,
            cpu,
            group,
            flags
        )
    );
}

#if defined(__x86_64__) || defined(__i386__)

inline uint64_t rdpmc(uint32_t counter) noexcept {
    uint32_t lo;
    uint32_t hi;

    asm volatile(
        "lfence\n\t"
        "rdpmc\n\t"
        "lfence"
        : "=a"(lo), "=d"(hi)
        : "c"(counter)
        : "memory"
    );

    return
        (static_cast<uint64_t>(hi) << 32) |
        static_cast<uint64_t>(lo);
}

inline uint64_t rdtsc() noexcept {
    uint32_t lo;
    uint32_t hi;

    asm volatile(
        "lfence\n\t"
        "rdtsc\n\t"
        "lfence"
        : "=a"(lo), "=d"(hi)
        :
        : "memory"
    );

    return
        (static_cast<uint64_t>(hi) << 32) |
        static_cast<uint64_t>(lo);
}

#endif

}

PerfEvent::PerfEvent(
    perf_event_attr attr,
    int pid,
    int cpu,
    int group,
    unsigned long flags
) {
    attr.size = sizeof(attr);

    fd_ = perfEventOpen(
        attr,
        pid,
        cpu,
        group,
        flags
    );

    if (fd_ < 0)
        return;

    map();
}

PerfEvent::~PerfEvent() {
    close();
}

PerfEvent::PerfEvent(
    PerfEvent&& other
) noexcept
    : fd_(other.fd_)
    , page_(other.page_) {

    other.fd_ = -1;
    other.page_ = nullptr;
}

PerfEvent& PerfEvent::operator=(
    PerfEvent&& other
) noexcept {
    if (this == &other)
        return *this;

    close();

    fd_ = other.fd_;
    page_ = other.page_;

    other.fd_ = -1;
    other.page_ = nullptr;

    return *this;
}

bool PerfEvent::fastReadable() const noexcept {
#if defined(__x86_64__) || defined(__i386__)
    return page_ && page_->cap_user_rdpmc;
#else
    return false;
#endif
}

void PerfEvent::reset(unsigned long flags) {
    if (!available())
        return;

    if (::ioctl(fd_, PERF_EVENT_IOC_RESET, flags) == -1) {
        throw Exception<PerfEvent>(
            "Failed to reset perf event: {}",
            std::strerror(errno)
        );
    }
}

void PerfEvent::enable(unsigned long flags) {
    if (!available())
        return;

    if (::ioctl(fd_, PERF_EVENT_IOC_ENABLE, flags) == -1) {
        throw Exception<PerfEvent>(
            "Failed to enable perf event: {}",
            std::strerror(errno)
        );
    }
}

void PerfEvent::disable(unsigned long flags) {
    if (!available())
        return;

    if (::ioctl(fd_, PERF_EVENT_IOC_DISABLE, flags) == -1) {
        throw Exception<PerfEvent>(
            "Failed to disable perf event: {}",
            std::strerror(errno)
        );
    }
}

PerfEvent::Snapshot PerfEvent::snapshot() const {
    if (!available())
        return {};

#if defined(__x86_64__) || defined(__i386__)
    if (fastReadable()) {
        return snapshotRdpmc();
    }
#endif

    return snapshotRead();
}

uint64_t PerfEvent::delta(
    const Snapshot& begin,
    const Snapshot& end
) {
    const uint64_t value =
        end.value - begin.value;

    const uint64_t enabled =
        end.timeEnabled - begin.timeEnabled;

    const uint64_t running =
        end.timeRunning - begin.timeRunning;

    if (enabled == running)
        return value;

    if (!running)
        return 0;

    const long double scaled =
        static_cast<long double>(value) *
        static_cast<long double>(enabled) /
        static_cast<long double>(running);

    return static_cast<uint64_t>(scaled);
}

PerfEvent::Snapshot PerfEvent::snapshotRdpmc() const noexcept {
#if defined(__x86_64__) || defined(__i386__)

    Snapshot result{};
    uint32_t index = 0;
    uint64_t cycles = 0;
    uint64_t timeOffset = 0;
    uint64_t timeCycles = 0;
    uint64_t timeMask = 0;
    uint32_t timeMult = 0;
    uint16_t timeShift = 0;
    bool userTime = false;
    bool userTimeShort = false;

    uint32_t sequence = 0;

    while (true) {
        sequence = __atomic_load_n(
            &page_->lock,
            __ATOMIC_ACQUIRE
        );

        if (sequence & 1)
            continue;

        index = page_->index;

        const int64_t offset =
            page_->offset;

        const uint16_t width =
            page_->pmc_width;

        result.timeEnabled =
            page_->time_enabled;

        result.timeRunning =
            page_->time_running;

        userTime =
            page_->cap_user_time &&
            result.timeEnabled != result.timeRunning;

        if (userTime) {
            cycles = rdtsc();
            timeOffset = page_->time_offset;
            timeMult = page_->time_mult;
            timeShift = page_->time_shift;
            userTimeShort = page_->cap_user_time_short;
            timeCycles = page_->time_cycles;
            timeMask = page_->time_mask;
        }

        int64_t value = offset;

        if (index) {
            int64_t pmc =
                static_cast<int64_t>(
                    rdpmc(index - 1)
                );

            if (width && width < 64) {
                pmc <<= 64 - width;
                pmc >>= 64 - width;
            }

            value += pmc;
        }

        result.value =
            static_cast<uint64_t>(value);

        __atomic_thread_fence(__ATOMIC_ACQUIRE);

        if (
            __atomic_load_n(
                &page_->lock,
                __ATOMIC_RELAXED
            ) == sequence
        ) {
            break;
        }
    }

    if (userTime) {
        if (userTimeShort) {
            cycles = timeCycles +
                ((cycles - timeCycles) & timeMask);
        }

        const uint64_t quotient =
            cycles >> timeShift;

        const uint64_t remainderMask =
            timeShift == 0
                ? 0
                : (uint64_t{1} << timeShift) - 1;

        const uint64_t remainder =
            cycles & remainderMask;

        const uint64_t delta =
            timeOffset +
            quotient * timeMult +
            ((remainder * timeMult) >> timeShift);

        result.timeEnabled += delta;

        if (index)
            result.timeRunning += delta;
    }

    return result;

#else
    return {};
#endif
}

PerfEvent::Snapshot PerfEvent::snapshotRead() const {
    struct ReadData {
        uint64_t value;
        uint64_t timeEnabled;
        uint64_t timeRunning;
    } data{};

    const ssize_t bytes =
        ::read(
            fd_,
            &data,
            sizeof(data)
        );

    if (bytes != static_cast<ssize_t>(sizeof(data))) {
        throw Exception<PerfEvent>(
            "Failed to read perf event: {}",
            std::strerror(errno)
        );
    }

    return {
        .value = data.value,
        .timeEnabled = data.timeEnabled,
        .timeRunning = data.timeRunning
    };
}

void PerfEvent::map() {
#if defined(__x86_64__) || defined(__i386__)

    const long pageSize =
        ::sysconf(_SC_PAGESIZE);

    if (pageSize <= 0)
        return;

    void* mapping =
        ::mmap(
            nullptr,
            static_cast<size_t>(pageSize),
            PROT_READ,
            MAP_SHARED,
            fd_,
            0
        );

    if (mapping == MAP_FAILED)
        return;

    page_ =
        static_cast<perf_event_mmap_page*>(
            mapping
        );

#endif
}

void PerfEvent::close() {
    if (page_) {
        const long pageSize =
            ::sysconf(_SC_PAGESIZE);

        if (pageSize > 0) {
            ::munmap(
                page_,
                static_cast<size_t>(pageSize)
            );
        }

        page_ = nullptr;
    }

    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

}
