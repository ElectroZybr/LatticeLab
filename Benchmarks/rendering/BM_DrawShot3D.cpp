#include <benchmark/benchmark.h>
#include "fixtures/RendererFixture.h"

#include "Rendering/3d/Renderer3D.h"

BENCHMARK_TEMPLATE_DEFINE_F(RendererFixture, DrawShot3D, Renderer3D)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atomStorage_, box_);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RendererFixture, DrawShot3D)
    ->RangeMultiplier(8)->Range(125, 8000);
