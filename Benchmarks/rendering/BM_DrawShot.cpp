#include <benchmark/benchmark.h>
#include "fixtures/RendererFixture.h"

BENCHMARK_DEFINE_F(RendererFixture, DrawShot)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atoms_, box_);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RendererFixture, DrawShot)
    ->RangeMultiplier(8)->Range(125, 8000);
