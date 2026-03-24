#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, ComputeForces)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        rebuildScene();
        state.ResumeTiming();

        StepOps::computeForces(
            simulation_->atoms, simulation_->sim_box,
            simulation_->forceField, Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atoms.data());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForces)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);