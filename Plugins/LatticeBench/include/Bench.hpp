#pragma once

#include <Lattice/Lattice.hpp>


class BenchSubsystem final : public Lattice::SubsystemAPI {
public:
    explicit BenchSubsystem(NodeBuild branch) {
        branch.action("bench", [this](Lattice::ActionContext& context) {
            runAll(context);
        });

        branch.action<std::string>(
            "run",
            [this](Lattice::ActionContext& context, std::string name) {
                run(context, name);
            }
        );

        branch.action("list", [this](Lattice::ActionContext& context) {
            list(context);
        });
    }

private:
    void runAll(Lattice::ActionContext& context);
    void run(Lattice::ActionContext& context, std::string_view name);
    void list(Lattice::ActionContext& context);
};