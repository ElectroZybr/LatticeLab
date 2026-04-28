#pragma once

#include <cmath>
#include <memory>

#include <benchmark/benchmark.h>

#include "App/interaction/ToolsManager.h"
#include "App/interaction/picking/PickingSystem.h"
#include "Engine/SimBox.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"
#include "Rendering/BaseRenderer.h"

template <typename T>
concept IsRenderer = std::derived_from<T, IRenderer>;

class RendererFixtureBase : public benchmark::Fixture {
protected:
    void prepareAtoms(benchmark::State& state);
    void createRenderTargets(wgpu::Device device, wgpu::TextureFormat colorFormat);
    void drawFrame();
    void setCounters(benchmark::State& state) const;

    std::unique_ptr<IRenderer> renderer_;
    AtomStorage atomStorage_;
    Bond::List bonds_;
    SimBox box_{Vec3f(300, 300, 300)};

private:
    static AtomStorage makeGridAtoms(int count);

    wgpu::Texture targetTexture_ = nullptr;
    wgpu::TextureView targetTextureView_ = nullptr;
    wgpu::Texture depthTexture_ = nullptr;
    wgpu::TextureView depthTextureView_ = nullptr;
};

wgpu::Device benchmarkDevice();
wgpu::TextureFormat benchmarkSurfaceFormat();

template <IsRenderer TRenderer> class RendererFixture : public RendererFixtureBase {
public:
    RendererFixture() = default;

    void SetUp(benchmark::State& state) override {
        prepareAtoms(state);

        renderer_ = std::make_unique<TRenderer>(box_, benchmarkDevice(), benchmarkSurfaceFormat());
        renderer_->camera.setScreenSize({800.0f, 600.0f});
        renderer_->camera.resetView();
        createRenderTargets(benchmarkDevice(), benchmarkSurfaceFormat());

        if (ToolsManager::pickingSystem) {
            delete ToolsManager::pickingSystem;
        }
        ToolsManager::pickingSystem = new PickingSystem(atomStorage_, box_, renderer_);
    }

    void TearDown(benchmark::State&) override {
        renderer_.reset();
    }
};
