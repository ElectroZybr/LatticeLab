#include <benchmark/benchmark.h>

#include "Benchmarks/fixtures/RendererFixture.h"
#include "Rendering/2d/Renderer2DBGFX.h"

// @bench_meta {"id":"RendererFixture<Renderer2D>/DrawShot2D","ru":"Отрисовка кадра 2D","group":"Рендер/2D"}
BENCHMARK_TEMPLATE_DEFINE_F(RendererFixture, DrawShot2D, Renderer2DBGFX)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atomStorage_, bonds_, box_);
        bgfx::frame();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RendererFixture, DrawShot2D)->RangeMultiplier(8)->Range(125, 8000);
