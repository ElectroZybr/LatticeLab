#include <benchmark/benchmark.h>

#include "Benchmarks/fixtures/RendererFixture.h"
#include "Rendering/3d/Renderer3DWGPU.h"

// @bench_meta {"id":"RendererFixture<Renderer3D>/DrawShot3D","ru":"Отрисовка кадра 3D","group":"Рендер/3D"}
BENCHMARK_TEMPLATE_DEFINE_F(RendererFixture, DrawShot3D, Renderer3DWGPU)(benchmark::State& state) {
    for (auto _ : state) {
        drawFrame();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RendererFixture, DrawShot3D)->RangeMultiplier(8)->Range(125, 8000);
