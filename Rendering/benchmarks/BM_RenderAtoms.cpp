#include <benchmark/benchmark.h>

#include "Rendering/benchmarks/Fixture.h"
#include "Rendering/benchmarks/SceneBuilders.h"

// @bench_meta {"id":"RenderFixture/RenderAtomsCpuPrepare","label":"Render Atoms CPU Prepare","group":"Rendering/Atoms"}
BENCHMARK_DEFINE_F(RenderFixture, RenderAtomsCpuPrepare)(benchmark::State& state) {
    RenderBenchScenes::buildAtoms(scene(), sceneArg());
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareAtomsCpuData(renderData().atoms, renderData(), false));
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderAtomsCpuToGpuCopy","label":"Render Atoms CPU->GPU Copy","group":"Rendering/Atoms"}
BENCHMARK_DEFINE_F(RenderFixture, RenderAtomsCpuToGpuCopy)(benchmark::State& state) {
    RenderBenchScenes::buildAtoms(scene(), sceneArg());
    syncRenderer();
    renderer_->prepareAtomsCpuData(renderData().atoms, renderData(), false);

    for (auto _ : state) {
        renderer_->uploadPreparedAtomsGpu(renderData().atoms, renderData());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderAtomsGpuDraw","label":"Render Atoms GPU Draw","group":"Rendering/Atoms"}
BENCHMARK_DEFINE_F(RenderFixture, RenderAtomsGpuDraw)(benchmark::State& state) {
    RenderBenchScenes::buildAtoms(scene(), sceneArg());
    syncRenderer();
    renderer_->prepareAtomsCpuData(renderData().atoms, renderData(), false);
    renderer_->uploadPreparedAtomsGpu(renderData().atoms, renderData());

    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedAtomsGpu(renderData().atoms.count);
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderAtomsFull","label":"Render Atoms Full","group":"Rendering/Atoms"}
BENCHMARK_DEFINE_F(RenderFixture, RenderAtomsFull)(benchmark::State& state) {
    RenderBenchScenes::buildAtoms(scene(), sceneArg());
    for (auto _ : state) {
        syncRenderer();
        renderFrame();
        benchmark::DoNotOptimize(renderer_->getRenderDataCount());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(RenderFixture, RenderAtomsCpuPrepare)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Arg(100000)
    ->Arg(1000000);

BENCHMARK_REGISTER_F(RenderFixture, RenderAtomsCpuToGpuCopy)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Arg(100000)
    ->Arg(1000000);

BENCHMARK_REGISTER_F(RenderFixture, RenderAtomsGpuDraw)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Arg(100000)
    ->Arg(1000000);

BENCHMARK_REGISTER_F(RenderFixture, RenderAtomsFull)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Arg(100000)
    ->Arg(1000000);
