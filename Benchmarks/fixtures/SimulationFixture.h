#pragma once

#include <benchmark/benchmark.h>
#include <cstdlib>
#include <memory>
#include <string>

#include "BenchmarkCase.h"
#include "BenchmarkScenes.h"
#include "Engine/Simulation.h"
#include "Engine/physics/integrators/StepOps.h"
#include "Engine/physics/integrators/VerletScheme.h"

namespace Benchmarks {
    constexpr double kDt      = 0.01;
    constexpr int    kAtomMin = 125;   // 5^3
    constexpr int    kAtomMax = 1000;  // 10^3

    inline SceneKind sceneFromEnv() {
        const char* raw = std::getenv("CHEM_BENCH_SCENE");
        if (raw == nullptr) {
            return SceneKind::Crystal3D;
        }
        const std::string value(raw);
        if (value == "crystal2d") {
            return SceneKind::Crystal2D;
        }
        if (value == "random_gas2d") {
            return SceneKind::RandomGas2D;
        }
        return SceneKind::Crystal3D;
    }

    inline BenchmarkCase makeCaseForSelectedScene(int atomCount) {
        BenchmarkCase benchmarkCase{
            .scene      = sceneFromEnv(),
            .integrator = Integrator::Scheme::Verlet,
            .atomCount  = atomCount,
            .boxSize     = Vec3f(160.0, 160.0, 160.0),
            .cellSize   = 5
        };

        if (benchmarkCase.scene == SceneKind::Crystal2D || benchmarkCase.scene == SceneKind::RandomGas2D) {
            benchmarkCase.boxSize = Vec3f(160.0, 160.0, 6.0);
        }
        return benchmarkCase;
    }
}

class SimulationFixture : public benchmark::Fixture {
public:
    void SetUp(benchmark::State& state) override {
        atomCount_  = static_cast<int>(state.range(0));
        box_ = std::make_unique<SimBox>(Vec3f(160, 160, 160));
        simulation_ = std::make_unique<Simulation>(*box_);
    }

    void TearDown(benchmark::State&) override {
        simulation_.reset();
    }

protected:
    void rebuildScene() {
        Benchmarks::BenchmarkScenes::build(
            *simulation_,
            Benchmarks::makeCaseForSelectedScene(atomCount_)
        );
    }

    void prepareForPredict() {
        rebuildScene();
        StepOps::computeForces(
            simulation_->atoms(), simulation_->box(),
            simulation_->forceField(), nullptr, Benchmarks::kDt
        );
    }

    void prepareNeighborList() {
        simulation_->neighborList().build(simulation_->atoms(), simulation_->box());
    }

    void prepareForCorrect() {
        prepareForPredict();
        StepOps::predictAndSync(
            simulation_->atoms(), simulation_->box(),
            Benchmarks::kDt, &VerletScheme::predict
        );
        StepOps::computeForces(
            simulation_->atoms(), simulation_->box(),
            simulation_->forceField(), nullptr, Benchmarks::kDt
        );
    }

    void setCounters(benchmark::State& state) const {
        const int64_t processedAtoms = simulation_
            ? static_cast<int64_t>(simulation_->atoms().size())
            : static_cast<int64_t>(atomCount_);
        state.SetItemsProcessed(
            state.iterations() * processedAtoms
        );
    }

    std::unique_ptr<SimBox> box_;
    std::unique_ptr<Simulation> simulation_;
    int atomCount_ = 0;
};
