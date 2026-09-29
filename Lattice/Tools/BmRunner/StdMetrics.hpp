#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

#include <Lattice/Tools/BmRunner/Stage.hpp>


namespace Lattice {

class Warmup : public StageDriver {
    using Clock = std::chrono::steady_clock;

public:
    size_t maxSamples = 32;
    std::chrono::microseconds target{10'000};

    void run(
        StageContext& context,
        std::span<StageCapability*> capabilities
    ) override {
        context.prepare();

        const auto firstStart = Clock::now();
        context.invoke(1);
        const auto firstEnd = Clock::now();

        const auto firstDuration = firstEnd - firstStart;

        size_t samples = 1;

        if (
            firstDuration > Clock::duration::zero() &&
            firstDuration < target
        ) {
            samples = std::clamp<size_t>(
                static_cast<size_t>(target / firstDuration),
                1,
                maxSamples
            );
        }

        context.progress("Warmup", 1, samples, {});

        for (size_t sample = 1; sample < samples; ++sample) {
            context.invoke(1);
            context.progress("Warmup", sample + 1, samples, {});
        }
    }
};


class Time : public StageCapability {
    using Clock = std::chrono::steady_clock;

    Clock::time_point start_;
    std::vector<double> samples_;

public:
    void begin(StageContext&) override {
        start_ = Clock::now();
    }

    void end(StageContext& context) override {
        const auto end = Clock::now();

        const double ns =
            std::chrono::duration<double, std::nano>(
                end - start_
            ).count() /
            static_cast<double>(context.iterations);

        samples_.push_back(ns);
    }

    void finish(StageContext& context) override {
        if (samples_.empty())
            return;

        std::sort(samples_.begin(), samples_.end());

        double sum = 0;
        for (double value : samples_)
            sum += value;

        context.result.add(
            "time.min",
            samples_.front(),
            Unit::Nanoseconds
        );

        context.result.add(
            "time.median",
            samples_[samples_.size() / 2],
            Unit::Nanoseconds
        );

        context.result.add(
            "time.mean",
            sum / static_cast<double>(samples_.size()),
            Unit::Nanoseconds
        );
    }
};

}