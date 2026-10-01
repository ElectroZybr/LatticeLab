#pragma once

#include <Lattice/Lattice.hpp>
#include <Lattice/Tools/BmRunner/Benchmarks.hpp>


class Benchmarks final : public Lattice::SubsystemAPI {
    Lattice::Children<Lattice::BasicTable> results_;
    size_t nextResult_ = 1;
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

    void configure(NodeConfigure branch) {
        results_ = branch.children<Lattice::BasicTable>();
    }

private:
    void onProgress(const Lattice::Benchmarks::SampleResult& progress);
    void writeResults(
        Lattice::ActionContext& context,
        std::string_view title,
        std::span<const Lattice::Benchmarks::BenchResult> results
    );
    void runAll(Lattice::ActionContext& context);
    void run(Lattice::ActionContext& context, std::string_view name);
    void list(Lattice::ActionContext& context);
};
