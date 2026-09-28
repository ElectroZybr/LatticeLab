#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>


#define BENCH1(name) \
    BENCH3("", name, "")

#define BENCH2(group, name) \
    BENCH3(group, name, "")

#define BENCH3(group, name, description) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(#group, #name, description, name); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH_SELECT(_1, _2, _3, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH3, BENCH2, BENCH1)(__VA_ARGS__)


namespace Lattice {

class Benchmarks {
    using BenchId = uint32_t;

public:
    class Bench;

    enum class Phase : uint8_t {
        Warmup,
        Sampling
    };

    static const char* phaseName(Phase phase) {
        return phase == Phase::Warmup ? "warmup" : "sample";
    }

    struct Progress {
        std::string_view name;
        std::string_view group;
        Phase phase = Phase::Sampling;
        size_t n = 0;
        size_t sample = 0;
        size_t samples = 0;
        size_t iterations = 0;
        double lastNs = 0;
    };

    using ProgressCallback = std::function<void(const Progress&)>;

    struct TimeValue {
        double value;
        const char* unit;
    };

    static TimeValue formatTime(double ns) {
        if (ns >= 1'000'000'000.0)
            return {ns / 1'000'000'000.0, "s"};
        if (ns >= 1'000'000.0)
            return {ns / 1'000'000.0, "ms"};
        if (ns >= 1'000.0)
            return {ns / 1'000.0, "us"};
        return {ns, "ns"};
    }

private:
    struct Case {
        std::string group;
        std::string name;
        std::string description;
        void (*function)(Bench&);
    };

    ObjectRegistry<Case, BenchId, std::string> benches_;
    ProgressCallback defaultProgress_ = [](const Progress& progress) {
        const TimeValue time = formatTime(progress.lastNs);

        char benchName[64];

        if (progress.group.empty()) {
            std::snprintf(
                benchName,
                sizeof(benchName),
                "%.*s",
                static_cast<int>(progress.name.size()),
                progress.name.data()
            );
        } else {
            std::snprintf(
                benchName,
                sizeof(benchName),
                "%.*s/%.*s",
                static_cast<int>(progress.group.size()),
                progress.group.data(),
                static_cast<int>(progress.name.size()),
                progress.name.data()
            );
        }

        char progressText[32];

        std::snprintf(
            progressText,
            sizeof(progressText),
            "%s=%zu/%zu",
            phaseName(progress.phase),
            progress.sample,
            progress.samples
        );

        std::fprintf(
            stdout,
            "\r%-15s N=%-8zu %-14s last=%7.2f %s",
            benchName,
            progress.n,
            progressText,
            time.value,
            time.unit
        );

        std::fflush(stdout);

        if (progress.sample == progress.samples)
            std::fprintf(stdout, "\n");
    };
    ProgressCallback progress_ = defaultProgress_;

public:
    struct Result {
        std::string name;
        std::string group;
        size_t n = 0;
        size_t iterations = 0;
        double minNs = 0;
        double medianNs = 0;
        double meanNs = 0;
    };

    struct Config {
        std::vector<size_t> sizes;
        size_t warmup = 32;
        size_t samples = 9;
        std::chrono::microseconds warmupTarget{10'000};
        std::chrono::microseconds target{50};
    };

    class Bench {
        using Clock = std::chrono::steady_clock;

        const Benchmarks& owner_;
        std::string group_;
        std::string name_;
        std::vector<Result> results_;

    public:
        Config config{};

        Bench(const Benchmarks& owner, std::string group, std::string name)
            : owner_(owner),
            group_(std::move(group)),
            name_(std::move(name)) {}

        template<typename Prepare, typename Function>
        void measure(Prepare&& prepare, Function&& function) {
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
            auto input = prepare(n);

            const auto firstWarmupStart = Clock::now();

            auto firstWarmupResult = function(input);
            doNotOptimize(firstWarmupResult);

            const auto firstWarmupEnd = Clock::now();
            const auto firstWarmupDuration = firstWarmupEnd - firstWarmupStart;

            size_t warmupSamples = 1;

            if (firstWarmupDuration > Clock::duration::zero() &&
                firstWarmupDuration < config.warmupTarget) {

                warmupSamples = std::clamp<size_t>(
                    static_cast<size_t>(config.warmupTarget / firstWarmupDuration),
                    1,
                    config.warmup
                );
            }

            owner_.progress({
                .name = name_,
                .group = group_,
                .phase = Phase::Warmup,
                .n = n,
                .sample = 1,
                .samples = warmupSamples,
                .iterations = 1,
                .lastNs = std::chrono::duration<double, std::nano>(
                    firstWarmupDuration
                ).count()
            });

            for (size_t sample = 1; sample < warmupSamples; ++sample) {
                const auto start = Clock::now();

                auto result = function(input);
                doNotOptimize(result);

                const auto end = Clock::now();
                const double ns = std::chrono::duration<double, std::nano>(end - start).count();

                owner_.progress({
                    .name = name_,
                    .group = group_,
                    .phase = Phase::Warmup,
                    .n = n,
                    .sample = sample + 1,
                    .samples = warmupSamples,
                    .iterations = 1,
                    .lastNs = ns
                });
            }

            size_t iterations = 1;

            while (true) {
                const auto start = Clock::now();

                for (size_t i = 0; i < iterations; ++i) {
                    auto result = function(input);
                    doNotOptimize(result);
                }

                if (Clock::now() - start >= config.target)
                    break;

                iterations *= 2;
            }

            std::vector<double> samples;
            samples.reserve(config.samples);

            for (size_t sample = 0; sample < config.samples; ++sample) {
                const auto start = Clock::now();

                for (size_t i = 0; i < iterations; ++i) {
                    auto result = function(input);
                    doNotOptimize(result);
                }

                const auto end = Clock::now();
                const double ns =
                    std::chrono::duration<double, std::nano>(end - start).count() /
                    static_cast<double>(iterations);

                samples.push_back(ns);

                owner_.progress({
                    .name = name_,
                    .group = group_,
                    .phase = Phase::Sampling,
                    .n = n,
                    .sample = sample + 1,
                    .samples = config.samples,
                    .iterations = iterations,
                    .lastNs = ns
                });
            }

            std::sort(samples.begin(), samples.end());

            double sum = 0;
            for (double value : samples)
                sum += value;

            return {
                .name = name_,
                .group = group_,
                .n = n,
                .iterations = iterations,
                .minNs = samples.front(),
                .medianNs = samples[samples.size() / 2],
                .meanNs = sum / static_cast<double>(samples.size())
            };
        }
    };

    class Registrar {
    public:
        Registrar(
            std::string_view group,
            std::string_view name,
            std::string_view description,
            void (*function)(Bench&)
        ) {
            Benchmarks::instance().add({
                std::string(group),
                std::string(name),
                std::string(description),
                function
            });
        }
    };

    static Benchmarks& instance() {
        static Benchmarks benchmarks;
        return benchmarks;
    }

    std::vector<Result> run(std::string_view name) const {
        const BenchId id = benches_.find(name);

        if (!benches_.valid(id))
            throw Exception<Benchmarks>("Benchmark '{}' not found", name);

        return execute(benches_.require(id));
    }

    std::vector<Result> runGroup(std::string_view group) const {
        std::vector<Result> results;

        for (BenchId id = 0; id < benches_.size(); ++id) {
            const Case* benchCase = benches_.get(id);

            if (!benchCase || benchCase->group != group)
                continue;

            append(results, execute(*benchCase));
        }

        return results;
    }

    std::vector<Result> runAll() const {
        std::vector<Result> results;

        for (BenchId id = 0; id < benches_.size(); ++id) {
            const Case* benchCase = benches_.get(id);

            if (benchCase)
                append(results, execute(*benchCase));
        }

        return results;
    }

    struct Info {
        std::string_view group;
        std::string_view name;
        std::string_view description;
    };

    std::vector<Info> list() const {
        std::vector<Info> result;

        for (BenchId id = 0; id < benches_.size(); ++id) {
            const Case* benchCase = benches_.get(id);

            if (!benchCase)
                continue;

            result.push_back({
                .group = benchCase->group,
                .name = benchCase->name,
                .description = benchCase->description
            });
        }

        return result;
    }

    std::vector<std::string_view> groups() const {
        std::vector<std::string_view> result;

        for (BenchId id = 0; id < benches_.size(); ++id) {
            const Case* benchCase = benches_.get(id);

            if (!benchCase || benchCase->group.empty())
                continue;

            if (std::find(result.begin(), result.end(), benchCase->group) == result.end())
                result.push_back(benchCase->group);
        }

        return result;
    }

    void setProgressCallback(ProgressCallback callback) {
        progress_ = std::move(callback);
    }

    void resetProgressCallback() { progress_ = defaultProgress_; }
    void disableProgress() { progress_ = {}; }

private:
    void add(Case bench) {
        benches_.create(std::move(bench));
    }

    void progress(const Progress& progress) const {
        if (progress_)
            progress_(progress);
    }

    std::vector<Result> execute(const Case& benchCase) const {
        Bench bench(*this, benchCase.group, benchCase.name);
        benchCase.function(bench);
        return bench.takeResults();
    }

    static void append(std::vector<Result>& destination, std::vector<Result> source) {
        destination.insert(
            destination.end(),
            std::make_move_iterator(source.begin()),
            std::make_move_iterator(source.end())
        );
    }
};

}