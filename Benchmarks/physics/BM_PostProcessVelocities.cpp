#include <benchmark/benchmark.h>

#include <vector>

#include "fixtures/SimulationFixture.h"

namespace {
struct VelocityPostProcessData {
    std::vector<float> baseVx;
    std::vector<float> baseVy;
    std::vector<float> baseVz;
};

VelocityPostProcessData prepareVelocityPostProcessData(Simulation& simulation) {
    VelocityPostProcessData data;
    const std::size_t mobileCount = simulation.atomStorage.mobileCount();

    data.baseVx.assign(simulation.atomStorage.vxData(), simulation.atomStorage.vxData() + mobileCount);
    data.baseVy.assign(simulation.atomStorage.vyData(), simulation.atomStorage.vyData() + mobileCount);
    data.baseVz.assign(simulation.atomStorage.vzData(), simulation.atomStorage.vzData() + mobileCount);
    return data;
}

void restoreVelocities(AtomStorage& atomStorage, const VelocityPostProcessData& data) {
    std::copy(data.baseVx.begin(), data.baseVx.end(), atomStorage.vxData());
    std::copy(data.baseVy.begin(), data.baseVy.end(), atomStorage.vyData());
    std::copy(data.baseVz.begin(), data.baseVz.end(), atomStorage.vzData());
}
}

// @bench_meta {"id":"SimulationFixture/PostProcessVelocities","ru":"Post-process скоростей: clamp","group":"Симуляция/Интегратор"}
BENCHMARK_DEFINE_F(SimulationFixture, PostProcessVelocities)(benchmark::State& state) {
    rebuildScene();
    VelocityPostProcessData data = prepareVelocityPostProcessData(*simulation_);

    for (auto _ : state) {
        state.PauseTiming();
        restoreVelocities(simulation_->atomStorage, data);
        state.ResumeTiming();

        StepOps::postProcessVelocities(simulation_->atomStorage, 1.0f);

        benchmark::DoNotOptimize(simulation_->atomStorage.vxData()[0]);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, PostProcessVelocities)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
