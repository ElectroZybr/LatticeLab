#include <chrono>
#include <cstddef>
#include <vector>
// #include <Lattice/Tools/Logger.hpp>


#define BENCH1(name) \
    BENCH2(name, "")

#define BENCH2(name, description) \
    static void name(::Lattice::Benchmark::Bench& bench); \
    static ::Lattice::Benchmark::BenchRegistrar _bench_##name(#name, description, name); \
    static void name(::Lattice::Benchmark::Bench& bench)

#define BENCH_SELECT(_1, _2, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH2, BENCH1)(__VA_ARGS__)


namespace Lattice::Benchmark {

struct Result {
    std::string name;
    size_t n = 0;
    size_t iterations = 0;
    double min_ns = 0;
    double med_ns = 0;
    double mean_ns = 0;
};

struct Config {
    std::vector<size_t> sizes;
    size_t warmup = 32;
    size_t samples = 9;
    std::chrono::microseconds target{50};
};

template<typename T>
inline void doNotOptimize(const T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(value) : "memory");
#else
    (void)value;
#endif
}

class Bench{
    using Clock = std::chrono::steady_clock;

    std::string name_;
    std::vector<Result> results_;
    
public:
    Config config{};

    explicit Bench(std::string name) : name_(std::move(name)) {}

    template<typename Prepare, typename Function>
    void measure(Prepare&& prepare, Function&& function) {
        results_.clear();
        results_.reserve(config.sizes.size());

        for (size_t n : config.sizes)
            results_.push_back(run(n, prepare, function));
    }

    const std::vector<Result>& results() const {
        return results_;
    }

private:
    template<typename Prepare, typename Function>
    Result run(size_t n, Prepare& prepare, Function& function) {
        auto input = prepare(n);

        for (size_t i = 0; i < config.warmup; ++i) {
            auto result = function(input);
            doNotOptimize(result);
        }

        size_t iterations = 1;

        while (true) {
            auto start = Clock::now();

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
            auto start = Clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                auto result = function(input);
                doNotOptimize(result);
            }

            auto end = Clock::now();
            double ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
            samples.push_back(ns);
        }

        std::sort(samples.begin(), samples.end());

        double sum = 0;
        for (double value : samples)
            sum += value;

        return {
            .name = name_,
            .n = n,
            .iterations = iterations,
            .min_ns = samples.front(),
            .med_ns = samples[samples.size() / 2],
            .mean_ns = sum / samples.size()
        };
    }
};

struct BenchCase {
    std::string name;
    std::string description;
    void (*function)(Bench&);
};

class Benchmarks {
    std::vector<BenchCase> benches_;

public:
    static Benchmarks& instance() {
        static Benchmarks benchmarks;
        return benchmarks;
    }

    void add(BenchCase bench) {
        benches_.push_back(std::move(bench));
    }

    const std::vector<BenchCase>& benches() const {
        return benches_;
    }

    int runAll() {
        for (const BenchCase& test : benches_) {
            Bench bench(test.name);
            test.function(bench);

            for (const Result& result : bench.results()) {
                std::printf(
                    "%-24s N=%-8zu median=%10.2f ns  min=%10.2f ns  mean=%10.2f ns  iterations=%zu\n",
                    result.name.c_str(),
                    result.n,
                    result.med_ns,
                    result.min_ns,
                    result.mean_ns,
                    result.iterations
                );
            }
        }

        return 0;
    }
};

class BenchRegistrar {
public:
    BenchRegistrar(std::string_view name, std::string_view description, void (*function)(Bench&)) {
        Benchmarks::instance().add({
            std::string(name),
            std::string(description),
            function
        });
    }
};

}