#pragma once

#include <algorithm>
#include <chrono>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Analysis.hpp>

namespace Lattice::Benchmarks {

class Time : public Capability {
    using Clock = std::chrono::steady_clock;

    static constexpr MetricDesc sampleSchema_[] = {
        {"time", Unit::Nanoseconds, MetricFlags::PerIteration | MetricFlags::Live}
    };

    static constexpr MetricDesc resultSchema_[] = {
        {"min", Unit::Nanoseconds, MetricFlags::PerIteration},
        {"median", Unit::Nanoseconds, MetricFlags::PerIteration | MetricFlags::Live},
        {"mean", Unit::Nanoseconds, MetricFlags::PerIteration}
    };

    Clock::time_point start_;
    Clock::duration elapsed_{};
    std::vector<double> samples_;

public:
    enum Metric : uint8_t {
        min,
        median,
        mean,
        _count
    };

    std::string_view name() const noexcept override {
        return "Time";
    }

    void begin() override {
        elapsed_ = {};
    }

    void start() override {
        start_ = Clock::now();
    }

    void stop() override {
        elapsed_ += Clock::now() - start_;
    }

    Metrics end() override {
        const double ns = std::chrono::duration<double, std::nano>(elapsed_).count();

        samples_.push_back(ns);

        return {
            .schema = sampleSchema_,
            .values = {ns}
        };
    }

    Metrics result() override {
        if (samples_.empty())
            return {};

        std::sort(samples_.begin(), samples_.end());

        double sum = 0;
        for (double value : samples_)
            sum += value;

        Metrics metrics{
            .schema = resultSchema_,
            .values = {
                samples_.front(),
                samples_[samples_.size() / 2],
                sum / static_cast<double>(samples_.size())
            }
        };

        samples_.clear();
        return metrics;
    }
};

constexpr ValueRef valueRef(Time::Metric metric) {
    return {
        .source = ValueSource::Metric,
        .capability = "Time",
        .index = static_cast<size_t>(metric)
    };
}

}