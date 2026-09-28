#include "Benchmarks.hpp"

#include <format>
#include <utility>

namespace {

template<typename Function>
auto runWithProgress(
    Lattice::Benchmarks& benchmarks,
    Lattice::Benchmarks::ProgressCallback progress,
    Function&& function
) {
    benchmarks.setProgressCallback(std::move(progress));

    struct ResetProgress {
        Lattice::Benchmarks& benchmarks;
        ~ResetProgress() { benchmarks.resetProgressCallback(); }
    } reset{benchmarks};

    return std::forward<Function>(function)();
}

std::string benchmarkName(std::string_view group, std::string_view name) {
    return group.empty() ? std::string(name) : std::format("{}/{}", group, name);
}

std::string formatTime(double nanoseconds) {
    const auto time = Lattice::Benchmarks::formatTime(nanoseconds);
    return std::format("{:.2f} {}", time.value, time.unit);
}

}

void Benchmarks::runAll(Lattice::ActionContext& context) {
    auto& benchmarks = Lattice::Benchmarks::instance();
    auto results = runWithProgress(
        benchmarks,
        [this](const auto& progress) { onProgress(progress); },
        [&benchmarks] { return benchmarks.runAll(); }
    );
    writeResults(context, "All benchmarks", results);
}

void Benchmarks::run(Lattice::ActionContext& context, std::string_view name) {
    auto& benchmarks = Lattice::Benchmarks::instance();
    auto results = runWithProgress(
        benchmarks,
        [this](const auto& progress) { onProgress(progress); },
        [&benchmarks, name] { return benchmarks.run(name); }
    );
    writeResults(context, name, results);
}

void Benchmarks::list(Lattice::ActionContext& context) {
    auto& benchmarks = Lattice::Benchmarks::instance();
    const auto groups = benchmarks.groups();
    const auto entries = benchmarks.list();
    Lattice::TreeFormatter tree("Benchmarks");

    for (const auto group : groups) {
        auto& groupNode = tree.branch(group);

        for (const auto& bench : entries) {
            if (bench.group != group)
                continue;

            if (bench.description.empty())
                groupNode.node(bench.name);
            else
                groupNode.node(Lattice::TextFormatter::format(
                    "<a2>{}</> <mut>({})</>",
                    bench.name,
                    bench.description
                ).markup());
        }
    }

    for (const auto& bench : entries) {
        if (!bench.group.empty())
            continue;

        if (bench.description.empty())
            tree.node(bench.name);
        else
            tree.node(Lattice::TextFormatter::format(
                "<a2>{}</> <mut>({})</>",
                bench.name,
                bench.description
            ).markup());
    }

    context.emit(Lattice::Value{tree.format().render()});
}

void Benchmarks::writeResults(
    Lattice::ActionContext& context,
    std::string_view title,
    std::span<const Lattice::Benchmarks::Result> results
) {
    if (results.empty()) {
        context.emit(Lattice::Value{std::format("{}: no results", title)});
        return;
    }

    Lattice::BasicTable table;
    table.addColumn<std::string>("benchmark");
    table.addColumn<uint64_t>("N");
    table.addColumn<std::string>("median");
    table.addColumn<std::string>("min");
    table.addColumn<std::string>("mean");
    table.addColumn<uint64_t>("iterations");

    for (const auto& result : results) {
        table.addRow(
            benchmarkName(result.group, result.name),
            static_cast<uint64_t>(result.n),
            formatTime(result.medianNs),
            formatTime(result.minNs),
            formatTime(result.meanNs),
            static_cast<uint64_t>(result.iterations)
        );
    }

    Lattice::TableFormatter formatter;
    Lattice::TableFormatter::Desc description;
    description.maxRows = Lattice::TableFormatter::Desc::Unlimited;

    Lattice::TextFormatter output = Lattice::TextFormatter::format("<a2>{}</>", title);
    for (const auto line : formatter.view(table, description)) {
        output.append("\n");
        output.append(line);
    }

    context.emit(Lattice::Value{output.render()});
}

void Benchmarks::onProgress(const Lattice::Benchmarks::Progress& progress) {
    const auto time = Lattice::Benchmarks::formatTime(progress.lastNs);
    const bool finished =
        progress.phase == Lattice::Benchmarks::Phase::Sampling &&
        progress.sample == progress.samples;

    auto line = Lattice::TextFormatter::format(
        "\r<mut><light><b>{:<15}<//> <a2>N</>: {:<8} "
        "<a>{:<6}</>: {:>4}/{:<4} <a2>last</>:{:>7.2f} {}",
        benchmarkName(progress.group, progress.name),
        progress.n,
        Lattice::Benchmarks::phaseName(progress.phase),
        progress.sample,
        progress.samples,
        time.value,
        time.unit
    ).render();

    if (finished)
        line += '\n';
    LogSystem::writeConsole(line);
}
