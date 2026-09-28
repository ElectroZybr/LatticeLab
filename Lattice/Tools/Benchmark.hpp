#pragma once

#include <chrono>
#include <cstddef>
#include <vector>


#define BENCH1(name) \
    BENCH3("", name, "")

#define BENCH2(group, name) \
    BENCH3(group, name, "")

#define BENCH3(group, name, description) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(group, #name, description, name); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH_SELECT(_1, _2, _3, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH3, BENCH2, BENCH1)(__VA_ARGS__)


namespace Lattice {

class Benchmarks {
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
        std::chrono::microseconds target{50};
    };

    class Bench {
        using Clock = std::chrono::steady_clock;

        std::string group_;
        std::string name_;
        std::vector<Result> results_;

    public:
        Config config{};

        Bench(std::string group, std::string name)
            : group_(std::move(group)),
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

            for (size_t i = 0; i < config.warmup; ++i) {
                auto result = function(input);
                doNotOptimize(result);
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

                samples.push_back(
                    std::chrono::duration<double, std::nano>(end - start).count() /
                    static_cast<double>(iterations)
                );
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
        const Case* benchCase = find(name);

        if (!benchCase)
            throw std::runtime_error(
                "Benchmark '" + std::string(name) + "' not found"
            );

        return execute(*benchCase);
    }

    std::vector<Result> runGroup(std::string_view group) const {
        std::vector<Result> results;

        for (const Case& benchCase : benches_) {
            if (benchCase.group != group)
                continue;

            append(results, execute(benchCase));
        }

        return results;
    }

    std::vector<Result> runAll() const {
        std::vector<Result> results;

        for (const Case& benchCase : benches_)
            append(results, execute(benchCase));

        return results;
    }

    std::vector<std::string_view> names() const {
        std::vector<std::string_view> result;
        result.reserve(benches_.size());

        for (const Case& benchCase : benches_)
            result.push_back(benchCase.name);

        return result;
    }

    std::vector<std::string_view> groups() const {
        std::vector<std::string_view> result;

        for (const Case& benchCase : benches_) {
            if (benchCase.group.empty())
                continue;

            if (std::find(result.begin(), result.end(), benchCase.group) == result.end())
                result.push_back(benchCase.group);
        }

        return result;
    }

    static void print(std::span<const Result> results, FILE* out = stdout) {
        for (const Result& result : results) {
            std::fprintf(
                out,
                "%-24s N=%-8zu median=%10.2f ns  min=%10.2f ns  mean=%10.2f ns  iterations=%zu\n",
                result.name.c_str(),
                result.n,
                result.medianNs,
                result.minNs,
                result.meanNs,
                result.iterations
            );
        }
    }

private:
    struct Case {
        std::string group;
        std::string name;
        std::string description;
        void (*function)(Bench&);
    };

    std::vector<Case> benches_;

    void add(Case bench) {
        benches_.push_back(std::move(bench));
    }

    const Case* find(std::string_view name) const {
        for (const Case& benchCase : benches_)
            if (benchCase.name == name)
                return &benchCase;

        return nullptr;
    }

    static std::vector<Result> execute(const Case& benchCase) {
        Bench bench(benchCase.group, benchCase.name);
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