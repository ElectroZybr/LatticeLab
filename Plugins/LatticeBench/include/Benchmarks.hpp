#pragma once

#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/Benchmark.hpp>

class Benchmarks final : public Lattice::SubsystemAPI {
public:
    explicit Benchmarks(NodeBuild branch) {
        branch.action<std::optional<std::string>>(
            "run",
            [this](
                Lattice::ActionContext& context,
                std::optional<std::string> name
            ) {
                if (name)
                    run(context, *name);
                else
                    runAll(context);
            }
        );

        branch.action("list", [this](Lattice::ActionContext& context) {
            list(context);
        });
    }

private:
    void onProgress(const Lattice::Benchmarks::Progress& progress);
    void writeResults(
        Lattice::ActionContext& context,
        std::string_view title,
        std::span<const Lattice::Benchmarks::Result> results
    );
    void runAll(Lattice::ActionContext& context);
    void run(Lattice::ActionContext& context, std::string_view name);
    void list(Lattice::ActionContext& context);
};
