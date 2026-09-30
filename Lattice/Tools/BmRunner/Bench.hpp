#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Metrics.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Lattice::Benchmarks {

/**
 @file Bench.hpp
 @brief Выполнение и измерение отдельного бенчмарка.

 Bench управляет параметрами запуска, подготовкой входных данных,
 калибровкой числа итераций и выполнением стадий измерения.

 Промежуточные и итоговые результаты передаются через callback-и
 после семплов, отдельных значений N и полного завершения бенчмарка.
*/

class Bench {
    using Clock = std::chrono::steady_clock;

public:
    struct Config {
        std::vector<size_t> sizes;
        size_t samples = 9;
        std::chrono::microseconds target{50};
    };

private:
    std::string group_;
    std::string name_;

    const SampleCallback& sample_;
    const ResultCallback& result_;
    const CompleteCallback& complete_;

public:
    Config config{};
    Stages stages;

    Bench(
        std::string group,
        std::string name,
        const SampleCallback& sample,
        const ResultCallback& result,
        const CompleteCallback& complete
    )
        : group_(std::move(group)),
          name_(std::move(name)),
          sample_(sample),
          result_(result),
          complete_(complete) {

        stages.add<Warmup>().samples(16).time(std::chrono::milliseconds(10));
        stages.add<Time>().samples(10);
    }

    std::string_view group() const noexcept {
        return group_;
    }

    std::string_view name() const noexcept {
        return name_;
    }

    template<typename Prepare, typename Function>
    void measure(Prepare&& prepare, Function&& function) {
        if (config.samples == 0)
            throw Exception<Bench>("Samples count cannot be zero");

        std::vector<PointResult> points;
        points.reserve(config.sizes.size());

        for (size_t n : config.sizes) {
            PointResult point = run(
                n,
                prepare,
                function
            );

            if (result_)
                result_(point);

            points.push_back(std::move(point));
        }

        if (complete_) {
            complete_({
                .name = name_,
                .group = group_,
                .points = points
            });
        }
    }

private:
    template<typename T>
    static void doNotOptimize(const T& value) {
    #if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : : "r,m"(value) : "memory");
    #else
        (void)value;
    #endif
    }

    static void normalize(Metrics& metrics, size_t iterations) {
        for (size_t i = 0; i < metrics.values.size(); ++i) {
            if (hasFlag(metrics.schema[i].flags, MetricFlags::PerIteration))
                metrics.values[i] /= static_cast<double>(iterations);
        }
    }

    template<typename Prepare, typename Function>
    PointResult run(
        size_t n,
        Prepare& prepare,
        Function& function
    ) {
        using Input = std::remove_cvref_t<std::invoke_result_t<Prepare&, size_t>>;

        std::optional<Input> input;
        input.emplace(std::invoke(prepare, n));

        auto invoke = [&](size_t iterations) {
            for (size_t i = 0; i < iterations; ++i) {
                if constexpr (
                    std::is_void_v<std::invoke_result_t<Function&, Input&>>
                ) {
                    std::invoke(function, *input);
                } else {
                    auto value = std::invoke(function, *input);
                    doNotOptimize(value);
                }
            }
        };

        PointResult point{
            .name = name_,
            .group = group_,
            .n = n
        };

        for (Stages::Stage& stage : stages.data())
            runStage(n, stage, invoke, point);

        return point;
    }

    template<typename Invoke>
    void runStage(
        size_t n,
        Stages::Stage& stage,
        Invoke& invoke,
        PointResult& point
    ) {
        if (!stage.sampleLimit && stage.timeLimit == Clock::duration::zero())
            throw Exception<Bench>("Stage '{}' has no execution limit", stageName(stage));

        const size_t iterations = calibrate(invoke);
        const std::string name = stageName(stage);

        const auto started = Clock::now();
        size_t sample = 0;

        while (true) {
            for (auto& capability : stage.capabilities)
                capability->begin();

            invoke(iterations);
            ++sample;

            SampleResult sampleResult{
                .name = name_,
                .group = group_,
                .stage = name,
                .n = n,
                .sample = sample,
                .samples = stage.sampleLimit,
                .iterations = iterations
            };

            sampleResult.capabilities.reserve(stage.capabilities.size());

            for (auto it = stage.capabilities.rbegin(); it != stage.capabilities.rend(); ++it) {
                Metrics metrics = (*it)->end();
                normalize(metrics, iterations);

                if (!metrics.values.empty()) {
                    sampleResult.capabilities.push_back({
                        .capability = (*it)->name(),
                        .metrics = std::move(metrics)
                    });
                }
            }

            if (sample_)
                sample_(sampleResult);

            const bool samplesReached =
                stage.sampleLimit && sample >= stage.sampleLimit;

            const bool timeReached =
                stage.timeLimit != Clock::duration::zero() &&
                Clock::now() - started >= stage.timeLimit;

            if (samplesReached || timeReached)
                break;
        }

        for (auto& capability : stage.capabilities) {
            Metrics metrics = capability->finish();
            normalize(metrics, iterations);

            if (!metrics.values.empty()) {
                point.capabilities.push_back({
                    .capability = capability->name(),
                    .metrics = std::move(metrics)
                });
            }
        }
    }

    template<typename Invoke>
    size_t calibrate(Invoke& invoke) {
        size_t iterations = 1;

        while (true) {
            const auto start = Clock::now();

            invoke(iterations);

            if (Clock::now() - start >= config.target)
                return iterations;

            if (
                iterations >
                std::numeric_limits<size_t>::max() / 2
            ) {
                throw Exception<Bench>("Iteration calibration overflow");
            }

            iterations *= 2;
        }
    }

    static std::string stageName(const Stages::Stage& stage) {
        std::string result;

        for (const auto& capability : stage.capabilities) {
            if (!result.empty())
                result += '|';

            result += capability->name();
        }

        return result;
    }
};

}