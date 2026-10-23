#include "Benchmarks.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include <BmRunner/Output.hpp>

namespace {

template<typename Function>
std::vector<Lattice::Benchmarks::BenchResult> runWithProgress(
    Lattice::Benchmarks::SampleCallback progress,
    Function&& function
) {
    std::vector<Lattice::Benchmarks::BenchResult> results;
    Lattice::Benchmarks::setSampleCallback(std::move(progress));
    Lattice::Benchmarks::disableResultCallback();
    Lattice::Benchmarks::setCompleteCallback(
        [&results](const Lattice::Benchmarks::BenchResult& result) {
            results.push_back(result);
        }
    );

    struct ResetCallbacks {
        ~ResetCallbacks() { Lattice::Benchmarks::resetCallbacks(); }
    } reset;

    std::forward<Function>(function)();
    return results;
}

std::string benchmarkName(std::string_view group, std::string_view name) {
    return group.empty() ? std::string(name) : std::format("{}/{}", group, name);
}

std::string resultName(std::string_view title, size_t index) {
    std::string name{title};
    for (char& character : name)
        if (character == '/' || character == ' ' || character == '\t')
            character = '-';

    if (name.empty())
        name = "results";

    return std::format("{}-{}", name, index);
}

}

void Benchmarks::run(Lattice::ActionContext& context, std::optional<std::string> name) {
    const auto progress = [this](const auto& sample) { onProgress(sample); };
    if (!name) {
        auto results = runWithProgress(progress, [] { Lattice::Benchmarks::runAll(); });
        writeResults(context, "All benchmarks", results);
        return;
    }
    auto results = runWithProgress(progress, [&name] { Lattice::Benchmarks::run(*name); });
    writeResults(context, *name, results);
}

void Benchmarks::view(Lattice::ActionContext& context, std::string name) const {
    for (const auto& [resultName, table] : results_)
        if (resultName == name) {
            context.present(static_cast<const Lattice::Table&>(table));
            return;
        }
    throw Lattice::Exception<Benchmarks>("Result '{}' not found", name);
}

void Benchmarks::list(Lattice::ActionContext& context) {
    const auto groups = Lattice::Benchmarks::groups();
    const auto entries = Lattice::Benchmarks::list();
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
    std::span<const Lattice::Benchmarks::BenchResult> results
) {
    if (results.empty()) {
        context.emit(Lattice::Value{std::format("{}: no results", title)});
        return;
    }

    auto& [name, stored] = results_.emplace_back(resultName(title, nextResult_++), Lattice::BasicTable{});
    auto* table = &stored;

    try {
        table->addColumn<std::string>("benchmark");
        table->addColumn<uint64_t>("N");
        table->addColumn<std::string>("stage");
        table->addColumn<std::string>("metric");
        table->addColumn<std::string>("value");

        for (const auto& result : results) {
            const std::string benchmark = benchmarkName(result.group, result.name);

            for (const auto& point : result.points) {
                for (const auto& stage : point.stages) {
                    for (const auto& capability : stage.capabilities) {
                        const auto count = std::min(
                            capability.metrics.schema.size(),
                            capability.metrics.values.size()
                        );

                        for (size_t index = 0; index < count; ++index) {
                            const auto& metric = capability.metrics.schema[index];
                            table->addRow(
                                benchmark,
                                static_cast<uint64_t>(point.n),
                                stage.name,
                                std::format("{}.{}", capability.capability, metric.name),
                                Lattice::Benchmarks::Output::formatValue(
                                    capability.metrics.values[index],
                                    metric.unit
                                )
                            );
                        }
                    }

                    for (const auto& unavailable : stage.unavailable) {
                        table->addRow(
                            benchmark,
                            static_cast<uint64_t>(point.n),
                            stage.name,
                            unavailable.capability,
                            std::format("unavailable: {}", unavailable.reason)
                        );
                    }
                }
            }
        }
    } catch (...) {
        results_.pop_back();
        throw;
    }

    Lattice::TableFormatter formatter;
    Lattice::TableFormatter::Desc description;
    description.maxRows = Lattice::TableFormatter::Desc::Unlimited;

    Lattice::TextFormatter output = Lattice::TextFormatter::format("<a2>{}</> <mut>({})</>", title, name);
    for (const auto line : formatter.view(*table, description)) {
        output.append("\n");
        output.append(line);
    }

    context.emit(Lattice::Value{output.render()});
}

void Benchmarks::onProgress(const Lattice::Benchmarks::SampleResult& progress) {
    auto line = Lattice::TextFormatter::format(
        "\r<mut><light><b>{:<15}<//> <a2>N</>: {:<8} "
        "<a>{:<12}</>: {:>4}",
        benchmarkName(progress.group, progress.name),
        progress.n,
        progress.stage,
        progress.sample
    ).render();

    if (progress.samples)
        line += std::format("/{:<4}", progress.samples);

    LogSystem::writeConsole(line);
}
