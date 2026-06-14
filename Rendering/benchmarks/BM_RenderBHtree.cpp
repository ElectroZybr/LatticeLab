#include <benchmark/benchmark.h>

#include "Rendering/benchmarks/Fixture.h"
#include "Rendering/benchmarks/SceneBuilders.h"

// @bench_meta {"id":"RenderFixture/RenderBHtreePrepare","label":"Render BHtree Prepare","group":"Rendering/BHtree"}
BENCHMARK_DEFINE_F(RenderFixture, RenderBHtreePrepare)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    scene().data.barnesHutTree = scene().data.grid;
    scene().data.grid = {};
    scene().data.drawGrid = false;
    scene().data.drawBHtree = true;
    syncRenderer();
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderer_->prepareGridCpuData(scene().data.barnesHutTree));
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderBHtreeCopy","label":"Render BHtree CPU->GPU Copy","group":"Rendering/BHtree"}
BENCHMARK_DEFINE_F(RenderFixture, RenderBHtreeCopy)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    scene().data.barnesHutTree = scene().data.grid;
    scene().data.grid = {};
    scene().data.drawGrid = false;
    scene().data.drawBHtree = true;
    syncRenderer();
    renderer_->prepareGridCpuData(scene().data.barnesHutTree);
    for (auto _ : state) {
        renderer_->uploadPreparedGridGpu();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderBHtreeDraw","label":"Render BHtree GPU Draw","group":"Rendering/BHtree"}
BENCHMARK_DEFINE_F(RenderFixture, RenderBHtreeDraw)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    scene().data.barnesHutTree = scene().data.grid;
    scene().data.grid = {};
    scene().data.drawGrid = false;
    scene().data.drawBHtree = true;
    syncRenderer();
    renderer_->prepareGridCpuData(scene().data.barnesHutTree);
    renderer_->uploadPreparedGridGpu();
    for (auto _ : state) {
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedGridGpu(renderData());
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"RenderFixture/RenderBHtreeFull","label":"Render BHtree Full","group":"Rendering/BHtree"}
BENCHMARK_DEFINE_F(RenderFixture, RenderBHtreeFull)(benchmark::State& state) {
    RenderBenchScenes::buildGrid(scene(), sceneArg());
    scene().data.barnesHutTree = scene().data.grid;
    scene().data.grid = {};
    scene().data.drawGrid = false;
    scene().data.drawBHtree = true;
    for (auto _ : state) {
        syncRenderer();
        renderer_->prepareGridCpuData(scene().data.barnesHutTree);
        renderer_->uploadPreparedGridGpu();
        renderer_->setupSceneUniforms(renderData());
        renderer_->beginPass(*renderTargetView_, *WGPUContext::instance().depthView(), wgpu::LoadOp::Clear);
        renderer_->drawPreparedGridGpu(renderData());
        renderer_->endFrame();
        WGPUContext::instance().waitIdle();
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(RenderFixture, RenderBHtreePrepare)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderBHtreeCopy)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderBHtreeDraw)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_REGISTER_F(RenderFixture, RenderBHtreeFull)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
