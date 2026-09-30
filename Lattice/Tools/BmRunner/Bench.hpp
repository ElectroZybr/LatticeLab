#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/Metrics.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/Fixture.hpp>

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

    Fixture::Factory createFixture_;
    const SampleCallback& sample_;
    const ResultCallback& result_;
    const CompleteCallback& complete_;

public:
    Config config{};
    Stages stages;

    Bench(
        std::string_view group,
        std::string_view name,
        Fixture::Factory createFixture,
        const SampleCallback& sample,
        const ResultCallback& result,
        const CompleteCallback& complete
    )
        : group_(group)
        , name_(name)
        , createFixture_(createFixture)
        , sample_(sample)
        , result_(result)
        , complete_(complete) {

        stages.add<Warmup>().samples(16).time(std::chrono::milliseconds(10));
        stages.add<Time>().samples(10);
    }

    std::string_view group() const noexcept {
        return group_;
    }

    std::string_view name() const noexcept {
        return name_;
    }

    template<typename FixtureType, typename Function>
    void measure(Function&& function) {
        if (config.samples == 0)
            throw Exception<Bench>("Samples count cannot be zero");

        std::vector<PointResult> points;
        points.reserve(config.sizes.size());

        auto invoke = [&](Fixture& fixture) {
            auto& typed = static_cast<FixtureType&>(fixture);

            if constexpr (std::is_void_v<std::invoke_result_t<Function&, FixtureType&>>) {
                std::invoke(function, typed);
            } else {
                auto value = std::invoke(function, typed);
                doNotOptimize(value);
            }
        };

        for (size_t n : config.sizes) {
            PointResult point = run(n, invoke);

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

    template<typename Invoke>
    PointResult run(size_t n, Invoke& invoke) {
        PointResult point{
            .name = name_,
            .group = group_,
            .n = n
        };

        for (Stages::Stage& stage : stages.data())
            runStage(n, stage, invoke, point);

        return point;
    }

    Clock::duration measureOverhead(Stages::Stage& stage) {
        constexpr size_t rounds = 16;
        constexpr size_t iterations = 1000;

        Clock::duration best = Clock::duration::max();

        for (size_t round = 0; round < rounds; ++round) {
            const auto started = Clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                for (auto& capability : stage.capabilities)
                    capability->start();

                for (auto it = stage.capabilities.rbegin(); it != stage.capabilities.rend(); ++it)
                    (*it)->stop();
            }

            const auto elapsed = Clock::now() - started;
            const auto average = elapsed / static_cast<Clock::rep>(iterations);
            best = std::min(best, average);
        }

        return best;
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

        const std::string name = stageName(stage);
        const auto overhead = measureOverhead(stage);
        const size_t iterations = calibrate(n, invoke);

        StageResult stageResult{
            .name = name,
        };

        const auto started = Clock::now();
        size_t sample = 0;

        while (true) {
            auto fixture = createFixture_(n);

            for (auto& capability : stage.capabilities)
                capability->begin();

            for (size_t i = 0; i < iterations; ++i) {
                fixture->prepare();

                for (auto& capability : stage.capabilities)
                    capability->start();

                invoke(*fixture);

                for (auto it = stage.capabilities.rbegin(); it != stage.capabilities.rend(); ++it)
                    (*it)->stop();
            }

            ++sample;

            SampleResult sampleResult{
                .name = name_,
                .group = group_,
                .stage = name,
                .n = n,
                .sample = sample,
                .samples = stage.sampleLimit,
                .iterations = iterations,
                .overhead = std::chrono::duration<double, std::nano>(overhead).count()
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

        stageResult.capabilities.reserve(stage.capabilities.size());

        for (auto& capability : stage.capabilities) {
            Metrics metrics = capability->result();
            normalize(metrics, iterations);

            if (!metrics.values.empty()) {
                stageResult.capabilities.push_back({
                    .capability = capability->name(),
                    .metrics = std::move(metrics)
                });
            }
        }

        point.stages.push_back(std::move(stageResult));
    }

    template<typename Invoke>
    size_t calibrate(size_t n, Invoke& invoke) {
        auto fixture = createFixture_(n);

        size_t iterations = 1;

        while (true) {
            Clock::duration elapsed{};

            for (size_t i = 0; i < iterations; ++i) {
                fixture->prepare();

                const auto start = Clock::now();
                invoke(*fixture);
                elapsed += Clock::now() - start;
            }

            if (elapsed >= config.target)
                return iterations;

            const double scale =
                static_cast<double>(config.target.count()) /
                static_cast<double>(
                    std::chrono::duration_cast<decltype(config.target)>(elapsed).count()
                );

            iterations = std::max(
                iterations + 1,
                static_cast<size_t>(iterations * scale)
            );
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