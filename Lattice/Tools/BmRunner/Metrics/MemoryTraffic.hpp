#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include <Lattice/Tools/BmRunner/Analysis.hpp>
#include <Lattice/Tools/BmRunner/Stages.hpp>

namespace Lattice::Benchmarks {

class MemoryTrafficBackend {
public:
    virtual ~MemoryTrafficBackend() = default;

    virtual bool available() const noexcept = 0;
    virtual std::string_view unavailableReason() const noexcept {
        return {};
    }

    virtual void begin() = 0;
    virtual uint64_t end() = 0;
};

class MemoryTraffic : public Capability {
    using Clock = std::chrono::steady_clock;

    static constexpr MetricDesc schema_[] = {
        {"memory", Unit::Bytes, MetricFlags::PerIteration | MetricFlags::Live},
        {"bandwidth", Unit::BytesPerSecond, MetricFlags::Live}
    };

    std::unique_ptr<MemoryTrafficBackend> backend_;
    Clock::time_point started_;
    Clock::duration totalElapsed_{};
    long double totalBytes_ = 0.0;
    size_t samples_ = 0;
    bool measuring_ = false;

public:
    enum Metric : uint8_t {
        bytes,
        bandwidth,
        _count
    };

    MemoryTraffic();

    explicit MemoryTraffic(
        std::unique_ptr<MemoryTrafficBackend> backend
    )
        : backend_(std::move(backend)) {}

    std::string_view name() const noexcept override {
        return "MemoryTraffic";
    }

    void begin() override {
        measuring_ = false;

        if (!available())
            return;

        backend_->begin();
        started_ = Clock::now();
        measuring_ = true;
    }

    Metrics end() override {
        if (!available() || !measuring_)
            return {};

        const Clock::duration elapsed = Clock::now() - started_;
        measuring_ = false;

        const uint64_t measuredBytes = backend_->end();
        const double bytes = static_cast<double>(measuredBytes);
        const double bandwidth = bytesPerSecond(bytes, elapsed);

        totalBytes_ += static_cast<long double>(measuredBytes);
        totalElapsed_ += elapsed;
        ++samples_;

        return {
            .schema = schema_,
            .values = {bytes, bandwidth}
        };
    }

    Metrics result() override {
        if (samples_ == 0)
            return {};

        const double meanBytes = static_cast<double>(
            totalBytes_ / static_cast<long double>(samples_)
        );
        const double bandwidth = bytesPerSecond(
            static_cast<double>(totalBytes_),
            totalElapsed_
        );

        totalBytes_ = 0.0;
        totalElapsed_ = {};
        samples_ = 0;

        return {
            .schema = schema_,
            .values = {meanBytes, bandwidth}
        };
    }

    bool available() const noexcept override {
        return backend_ && backend_->available();
    }

    std::string_view unavailableReason() const noexcept override {
        return backend_
            ? backend_->unavailableReason()
            : "memory traffic backend is not configured";
    }

private:
    static double bytesPerSecond(double bytes, Clock::duration elapsed) {
        const double seconds = std::chrono::duration<double>(elapsed).count();
        return seconds > 0.0 ? bytes / seconds : 0.0;
    }
};

constexpr ValueRef valueRef(MemoryTraffic::Metric metric) {
    return {
        .source = ValueSource::Metric,
        .capability = "MemoryTraffic",
        .index = static_cast<size_t>(metric)
    };
}

}
