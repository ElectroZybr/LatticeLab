#include "Benchmarks.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include <Lattice/Tools/BmRunner/Bench.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Lattice {

Benchmarks::Benchmarks()
    : defaultProgress_(defaultProgress),
      progress_(defaultProgress_) {}

Benchmarks::Registrar::Registrar(
    std::string_view group,
    std::string_view name,
    std::string_view description,
    void (*function)(Bench&)
) {
    Benchmarks::instance().add({
        .group = std::string(group),
        .name = std::string(name),
        .description = std::string(description),
        .function = function
    });
}

Benchmarks& Benchmarks::instance() {
    static Benchmarks benchmarks;
    return benchmarks;
}

Benchmarks::Run Benchmarks::run(std::string_view name) const {
    const BenchId id = benches_.find(name);

    if (!benches_.valid(id))
        throw Exception<Benchmarks>("Benchmark '{}' not found", name);

    return execute(benches_.require(id));
}

std::vector<Benchmarks::Run> Benchmarks::runGroup(std::string_view group) const {
    std::vector<Run> result;

    for (BenchId id = 0; id < benches_.size(); ++id) {
        const Case* benchCase = benches_.get(id);

        if (!benchCase || benchCase->group != group)
            continue;

        result.push_back(execute(*benchCase));
    }

    return result;
}

std::vector<Benchmarks::Run> Benchmarks::runAll() const {
    std::vector<Run> result;

    for (BenchId id = 0; id < benches_.size(); ++id) {
        const Case* benchCase = benches_.get(id);

        if (benchCase)
            result.push_back(execute(*benchCase));
    }

    return result;
}

std::vector<Benchmarks::Info> Benchmarks::list() const {
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

std::vector<std::string_view> Benchmarks::groups() const {
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

void Benchmarks::setProgressCallback(ProgressCallback callback) {
    progress_ = std::move(callback);
}

void Benchmarks::resetProgressCallback() {
    progress_ = defaultProgress_;
}

void Benchmarks::disableProgress() {
    progress_ = {};
}

void Benchmarks::print(const Run& run, FILE* out) {
    std::fprintf(out, "%s\n", fullName(run.group, run.name).c_str());

    for (const Result& result : run.results) {
        std::fprintf(out, "  ");

        for (size_t i = 0; i < result.metrics.size(); ++i) {
            const Metric& metric = result.metrics[i];
            const std::string value = Bench::formatMetric(metric);

            if (i != 0)
                std::fprintf(out, "  ");

            std::fprintf(
                out,
                "%s=%s",
                metric.name.c_str(),
                value.c_str()
            );
        }

        std::fprintf(out, "\n");
    }
}

void Benchmarks::print(std::span<const Run> runs, FILE* out) {
    for (size_t i = 0; i < runs.size(); ++i) {
        print(runs[i], out);

        if (i + 1 < runs.size())
            std::fprintf(out, "\n");
    }
}

void Benchmarks::add(Case bench) {
    benches_.create(std::move(bench));
}

Benchmarks::Run Benchmarks::execute(const Case& benchCase) const {
    Bench bench(
        benchCase.group,
        benchCase.name,
        progress_
    );

    benchCase.function(bench);

    return {
        .name = benchCase.name,
        .group = benchCase.group,
        .results = bench.takeResults()
    };
}

void Benchmarks::defaultProgress(const Progress& progress) {
    const std::string name = fullName(progress.group, progress.name);

    std::fprintf(
        stdout,
        "\r%-20s %-10.*s %4zu/%-4zu",
        name.c_str(),
        static_cast<int>(progress.stage.size()),
        progress.stage.data(),
        progress.current,
        progress.total
    );

    for (const Metric& metric : progress.metrics) {
        const std::string value = Bench::formatMetric(metric);

        std::fprintf(
            stdout,
            "  %s=%s",
            metric.name.c_str(),
            value.c_str()
        );
    }

    std::fflush(stdout);

    if (progress.current == progress.total)
        std::fprintf(stdout, "\n");
}

std::string Benchmarks::fullName(
    std::string_view group,
    std::string_view name
) {
    if (group.empty())
        return std::string(name);

    return std::format("{}/{}", group, name);
}

}