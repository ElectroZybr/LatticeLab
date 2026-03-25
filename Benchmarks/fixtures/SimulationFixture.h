#pragma once

#include <benchmark/benchmark.h>
#include <memory>

#include "BenchmarkCase.h"
#include "BenchmarkScenes.h"
#include "Engine/Simulation.h"
#include "Engine/physics/integrators/StepOps.h"
#include "Engine/physics/integrators/VerletScheme.h"

namespace Benchmarks {
    constexpr double kDt      = 0.01;
    constexpr int    kAtomMin = 125;   // 5^3
    constexpr int    kAtomMax = 1000;  // 10^3

    inline BenchmarkCase makeCrystal3DCase(int atomCount) {
        return BenchmarkCase{
            .scene      = SceneKind::Crystal3D,
            .integrator = Integrator::Scheme::Verlet,
            .atomCount  = atomCount,
            .boxStart   = Vec3f(-80.0, -80.0, -80.0),
            .boxEnd     = Vec3f( 80.0,  80.0,  80.0),
            .cellSize   = 5
        };
    }
}

class SimulationFixture : public benchmark::Fixture {
public:
    void SetUp(benchmark::State& state) override {
        atomCount_  = static_cast<int>(state.range(0));
        box_ = std::make_unique<SimBox>(Vec3f(-80, -80, -80), Vec3f(80, 80, 80));
        simulation_ = std::make_unique<Simulation>(*box_);
    }

    void TearDown(benchmark::State&) override {
        simulation_.reset();
    }

protected:
    void rebuildScene() {
        Benchmarks::BenchmarkScenes::build(
            *simulation_,
            Benchmarks::makeCrystal3DCase(atomCount_)
        );
    }

    void prepareForPredict() {
        rebuildScene();
        StepOps::computeForces(
            simulation_->atomStorage, simulation_->sim_box,
            simulation_->forceField, Benchmarks::kDt
        );
    }

    void prepareForCorrect() {
        prepareForPredict();
        StepOps::predictAndSync(
            simulation_->atomStorage, simulation_->sim_box,
            Benchmarks::kDt, &VerletScheme::predict
        );
        StepOps::computeForces(
            simulation_->atomStorage, simulation_->sim_box,
            simulation_->forceField, Benchmarks::kDt
        );
    }

    void setCounters(benchmark::State& state) const {
        state.SetItemsProcessed(
            state.iterations() * static_cast<int64_t>(atomCount_)
        );
    }

    std::unique_ptr<SimBox> box_;
    std::unique_ptr<Simulation> simulation_;
    int atomCount_ = 0;
};
