#pragma once

#include <algorithm>
#include <chrono>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/Stages.hpp>

namespace Lattice::Benchmarks {

class Time : public MetricCapability<"Time", Time> {
    using Clock = std::chrono::steady_clock;

    static constexpr MetricDesc sampleSchema_[] = {
        {"time", Unit::Nanoseconds, MetricFlags::PerIteration | MetricFlags::Live}
    };

    Clock::time_point start_;
    Clock::duration elapsed_{};
    std::vector<double> samples_;

public:
    inline static constexpr auto min = defineMetric(
        "min", Unit::Nanoseconds, MetricFlags::PerIteration
    );
    inline static constexpr auto median = defineMetric(
        "median",
        Unit::Nanoseconds,
        MetricFlags::PerIteration | MetricFlags::Live
    );
    inline static constexpr auto mean = defineMetric(
        "mean", Unit::Nanoseconds, MetricFlags::PerIteration
    );

    inline static constexpr auto Schema = defineSchema(min, median, mean);

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
            .schema = schema(),
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

}
