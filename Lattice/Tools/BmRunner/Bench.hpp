#pragma once

#include <chrono>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/Stages.hpp>
#include <Lattice/Tools/BmRunner/StdMetrics.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Lattice {

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
    const ProgressCallback& progress_;
    std::vector<Result> results_;

public:
    Config config{};
    Stages stages;

    Bench(
        std::string group,
        std::string name,
        const ProgressCallback& progress
    )
        : group_(std::move(group)),
          name_(std::move(name)),
          progress_(progress) {

        stages
            .add<Warmup>()
            .add<Time>();
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

        results_.clear();
        results_.reserve(config.sizes.size());

        for (size_t n : config.sizes)
            results_.push_back(run(n, prepare, function));
    }

    std::vector<Result> takeResults() {
        return std::move(results_);
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

    template<typename Prepare, typename Function>
    Result run(size_t n, Prepare& prepare, Function& function) {
        using Input = decltype(prepare(n));

        std::optional<Input> input;
        Result result;

        result.add("N", static_cast<double>(n), Unit::Count);

        StageContext context{
            .n = n,
            .samples = config.samples,
            .iterations = 1,
            .result = result
        };

        context.prepare = [&] {
            input.emplace(prepare(n));
        };

        context.invoke = [&](size_t iterations) {
            for (size_t i = 0; i < iterations; ++i) {
                auto value = function(*input);
                doNotOptimize(value);
            }
        };

        context.progress = [&](
            std::string_view stage,
            size_t current,
            size_t total,
            std::span<const Metric> metrics
        ) {
            progress({
                .name = name_,
                .group = group_,
                .stage = stage,
                .current = current,
                .total = total,
                .metrics = {
                    metrics.begin(),
                    metrics.end()
                }
            });
        };

        for (auto& stage : stages.data())
            runStage(stage, context);

        return result;
    }

    void runStage(
        Stages::Stage& stage,
        StageContext& context
    ) {
        std::vector<StageCapability*> capabilities;
        capabilities.reserve(stage.capabilities.size());

        StageDriver* driver = nullptr;

        for (auto& capability : stage.capabilities) {
            StageCapability* ptr = capability.get();

            capabilities.push_back(ptr);

            if (auto* candidate = dynamic_cast<StageDriver*>(ptr)) {
                if (driver)
                    throw Exception<Bench>(
                        "Stage contains multiple drivers"
                    );

                driver = candidate;
            }
        }

        if (driver) {
            driver->run(context, capabilities);
            return;
        }

        runSamples(context, capabilities);
    }

    void runSamples(
        StageContext& context,
        std::span<StageCapability*> capabilities
    ) {
        context.prepare();
        context.iterations = calibrate(context);

        for (size_t sample = 0; sample < context.samples; ++sample) {
            for (StageCapability* capability : capabilities)
                capability->begin(context);

            context.invoke(context.iterations);

            for (auto it = capabilities.rbegin(); it != capabilities.rend(); ++it)
                (*it)->end(context);

            context.progress(
                stageName(capabilities),
                sample + 1,
                context.samples,
                {}
            );
        }

        for (StageCapability* capability : capabilities)
            capability->finish(context);
    }

    size_t calibrate(StageContext& context) {
        size_t iterations = 1;

        while (true) {
            const auto start = Clock::now();

            context.invoke(iterations);

            if (Clock::now() - start >= config.target)
                return iterations;

            if (iterations > std::numeric_limits<size_t>::max() / 2)
                throw Exception<Bench>(
                    "Iteration calibration overflow"
                );

            iterations *= 2;
        }
    }

    static std::string stageName(
        std::span<StageCapability*> capabilities
    ) {
        // Пока можно вернуть просто "Measure".
        // Следом лучше добавить name() в capability через typeName<T>().
        return "Measure";
    }

    void progress(const Progress& value) const {
        if (progress_)
            progress_(value);
    }

    static std::string formatTime(double ns) {
        if (ns >= 1'000'000'000.0)
            return std::format("{:.2f} s", ns / 1'000'000'000.0);

        if (ns >= 1'000'000.0)
            return std::format("{:.2f} ms", ns / 1'000'000.0);

        if (ns >= 1'000.0)
            return std::format("{:.2f} us", ns / 1'000.0);

        return std::format("{:.2f} ns", ns);
    }

    static std::string formatBytes(double bytes) {
        if (bytes >= 1024.0 * 1024.0 * 1024.0)
            return std::format("{:.2f} GiB", bytes / (1024.0 * 1024.0 * 1024.0));

        if (bytes >= 1024.0 * 1024.0)
            return std::format("{:.2f} MiB", bytes / (1024.0 * 1024.0));

        if (bytes >= 1024.0)
            return std::format("{:.2f} KiB", bytes / 1024.0);

        return std::format("{:.0f} B", bytes);
    }
};

}