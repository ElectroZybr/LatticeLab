#include <benchmark/benchmark.h>
#include "fixtures/RendererFixture.h"

#include "Rendering/2d/Renderer2D.h"

BENCHMARK_TEMPLATE_DEFINE_F(RendererFixture, DrawShot2D, Renderer2D)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atomStorage_, box_);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RendererFixture, DrawShot2D)
    ->RangeMultiplier(8)->Range(125, 8000);
