#include <benchmark/benchmark.h>

#include "Rendering/benchmarks/Fixture.h"
#include "Rendering/benchmarks/SceneBuilders.h"

// @bench_meta {"id":"RenderFixture/RenderFieldPotentialPrepare","label":"Render Field Potential Prepare","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldPotentialPrepare)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), true, false, false);
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareFieldPotentialCpuData(renderData()));
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldPotentialCopy","label":"Render Field Potential CPU->GPU Copy","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldPotentialCopy)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), true, false, false);
    syncRenderer();
    renderer_->prepareFieldPotentialCpuData(renderData());
    for (auto _ : state) {
        renderer_->uploadPreparedFieldInstancesGpu();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldPotentialDraw","label":"Render Field Potential GPU Draw","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldPotentialDraw)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), true, false, false);
    syncRenderer();
    renderer_->prepareFieldPotentialCpuData(renderData());
    renderer_->uploadPreparedFieldInstancesGpu();
    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedFieldPotentialGpu();
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldPotentialFull","label":"Render Field Potential Full","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldPotentialFull)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), true, false, false);
    for (auto _ : state) {
        syncRenderer();
        renderFrame();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldArrowsPrepare","label":"Render Field Arrows Prepare","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldArrowsPrepare)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, true, false);
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareFieldArrowsCpuData(renderData()));
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldArrowsCopy","label":"Render Field Arrows CPU->GPU Copy","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldArrowsCopy)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, true, false);
    syncRenderer();
    renderer_->prepareFieldArrowsCpuData(renderData());
    for (auto _ : state) {
        renderer_->uploadPreparedFieldArrowsGpu(renderData().vectorField);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldArrowsDraw","label":"Render Field Arrows GPU Draw","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldArrowsDraw)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, true, false);
    syncRenderer();
    renderer_->prepareFieldArrowsCpuData(renderData());
    renderer_->uploadPreparedFieldArrowsGpu(renderData().vectorField);
    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedFieldArrowsGpu(renderData());
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldArrowsFull","label":"Render Field Arrows Full","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldArrowsFull)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, true, false);
    for (auto _ : state) {
        syncRenderer();
        renderFrame();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldContoursPrepare","label":"Render Field Contours Prepare","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldContoursPrepare)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, false, true);
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareFieldContoursCpuData(renderData()));
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldContoursCopy","label":"Render Field Contours CPU->GPU Copy","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldContoursCopy)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, false, true);
    syncRenderer();
    renderer_->prepareFieldContoursCpuData(renderData());
    for (auto _ : state) {
        renderer_->uploadPreparedFieldInstancesGpu();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldContoursDraw","label":"Render Field Contours GPU Draw","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldContoursDraw)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, false, true);
    syncRenderer();
    renderer_->prepareFieldContoursCpuData(renderData());
    renderer_->uploadPreparedFieldInstancesGpu();
    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedFieldContoursGpu();
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderFieldContoursFull","label":"Render Field Contours Full","group":"Rendering/Field"}
BENCHMARK_DEFINE_F(RenderFixture, RenderFieldContoursFull)(benchmark::State& state) {
    RenderBenchScenes::buildField(scene(), sceneArg(), false, false, true);
    for (auto _ : state) {
        syncRenderer();
        renderFrame();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RenderFixture, RenderFieldPotentialPrepare)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldPotentialCopy)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldPotentialDraw)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldPotentialFull)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldArrowsPrepare)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldArrowsCopy)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldArrowsDraw)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldArrowsFull)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldContoursPrepare)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldContoursCopy)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldContoursDraw)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
BENCHMARK_REGISTER_F(RenderFixture, RenderFieldContoursFull)->Arg(1024)->Arg(10000)->Arg(40000)->Arg(160000);
