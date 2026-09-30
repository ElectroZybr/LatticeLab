#pragma once

#include <algorithm>
#include <chrono>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/Stages.hpp>

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
    std::vector<double> samples_;

public:
    std::string_view name() const noexcept override {
        return "Time";
    }

    void begin() override {
        start_ = Clock::now();
    }

    Metrics end() override {
        const auto stop = Clock::now();

        const double ns =
            std::chrono::duration<double, std::nano>(
                stop - start_
            ).count();

        samples_.push_back(ns);

        return {
            .schema = sampleSchema_,
            .values = {ns}
        };
    }

    Metrics finish() override {
        if (samples_.empty())
            return {};

        std::sort(samples_.begin(), samples_.end());

        double sum = 0;
        for (double value : samples_)
            sum += value;

        Metrics result{
            .schema = resultSchema_,
            .values = {
                samples_.front(),
                samples_[samples_.size() / 2],
                sum / static_cast<double>(samples_.size())
            }
        };

        samples_.clear();
        return result;
    }
};

}