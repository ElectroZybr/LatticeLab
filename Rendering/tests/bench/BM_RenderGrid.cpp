#include <benchmark/benchmark.h>

#include "Rendering/benchmarks/Fixture.h"
#include "Rendering/benchmarks/SceneBuilder.h"

// @bench_meta {"id":"RenderFixture/RenderGridPrepare","label":"Render Grid Prepare","group":"Rendering/Grid/Stages"}
BENCHMARK_DEFINE_F(RenderFixture, RenderGridPrepare)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareGridCpuData(renderData().grid));
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderGridCopy","label":"Render Grid CPU->GPU Copy","group":"Rendering/Grid/Stages"}
BENCHMARK_DEFINE_F(RenderFixture, RenderGridCopy)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    syncRenderer();
    renderer_->prepareGridCpuData(renderData().grid);
    for (auto _ : state) {
        renderer_->uploadPreparedGridGpu();
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderGridDraw","label":"Render Grid GPU Draw","group":"Rendering/Grid/Stages"}
BENCHMARK_DEFINE_F(RenderFixture, RenderGridDraw)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    syncRenderer();
    renderer_->prepareGridCpuData(renderData().grid);
    renderer_->uploadPreparedGridGpu();
    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedGridGpu(renderData());
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        WGPUContext::instance().processEvents();
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderGridFull","label":"Render Grid Frame","group":"Rendering/Grid/Frame"}
BENCHMARK_DEFINE_F(RenderFixture, RenderGridFull)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    for (auto _ : state) {
        syncRenderer();
        renderFrame();
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(RenderFixture, RenderGridPrepare)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderGridCopy)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderGridDraw)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderGridFull)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
