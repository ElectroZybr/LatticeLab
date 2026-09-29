#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/Bench.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>

namespace Lattice {

class Benchmarks {
    using BenchId = uint32_t;

    struct Case {
        std::string group;
        std::string name;
        std::string description;
        void (*function)(Bench&);
    };

    ObjectRegistry<Case, BenchId, std::string> benches_;
    ProgressCallback defaultProgress_;
    ProgressCallback progress_;

public:
    struct Info {
        std::string_view group;
        std::string_view name;
        std::string_view description;
    };

    struct Run {
        std::string name;
        std::string group;
        std::vector<Result> results;
    };

    class Registrar {
    public:
        Registrar(
            std::string_view group,
            std::string_view name,
            std::string_view description,
            void (*function)(Bench&)
        );
    };

    Benchmarks();

    static Benchmarks& instance();

    Run run(std::string_view name) const;
    std::vector<Run> runGroup(std::string_view group) const;
    std::vector<Run> runAll() const;

    std::vector<Info> list() const;
    std::vector<std::string_view> groups() const;

    void setProgressCallback(ProgressCallback callback);
    void resetProgressCallback();
    void disableProgress();

    static void print(const Run& run, FILE* out = stdout);
    static void print(std::span<const Run> runs, FILE* out = stdout);

private:
    void add(Case bench);
    Run execute(const Case& benchCase) const;

    static void defaultProgress(const Progress& progress);
    static std::string fullName(std::string_view group, std::string_view name);
};

}

#define BENCH1(name) \
    static void name(::Lattice::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name("", #name, "", name); \
    static void name(::Lattice::Bench& bench)

#define BENCH2(group, name) \
    static void name(::Lattice::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(#group, #name, "", name); \
    static void name(::Lattice::Bench& bench)

#define BENCH3(group, name, description) \
    static void name(::Lattice::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(#group, #name, description, name); \
    static void name(::Lattice::Bench& bench)

#define BENCH_SELECT(_1, _2, _3, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH3, BENCH2, BENCH1)(__VA_ARGS__)