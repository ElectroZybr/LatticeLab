#pragma once

#include <cstdint>
#include <memory>

#include <benchmark/benchmark.h>

#include "Rendering/backend/WGPUContext.h"
#include "Rendering/benchmarks/SceneBuilders.h"
#include "Rendering/Renderer3D.h"

class RenderFixture : public benchmark::Fixture {
public:
    void SetUp(benchmark::State& state) override {
        initHeadlessContext();

        sceneArg_ = static_cast<size_t>(state.range(0));

        renderer_ = std::make_unique<Renderer3D>();
        renderer_->camera.setScreenSize(glm::vec2(kTargetWidth, kTargetHeight));
        renderer_->resizeRenderData(1);

        clearScene();
        syncRenderer();
        createRenderTarget();
    }

    void TearDown(benchmark::State&) override {
        renderTargetView_ = wgpu::raii::TextureView{};
        renderTarget_ = wgpu::raii::Texture{};
        renderer_.reset();
        RenderBenchScenes::clear(scene_);
    }

protected:
    static constexpr uint32_t kTargetWidth = 1280;
    static constexpr uint32_t kTargetHeight = 720;

    void clearScene() { RenderBenchScenes::clear(scene_); }

    void syncRenderer() {
        renderer_->getRenderData(0) = scene_.data;
        renderer_->camera.setSceneBounds(scene_.worldSize, scene_.renderOffset);
        renderer_->camera.resetView();
    }

    void renderFrame() {
        renderer_->drawShot(*renderTargetView_, *WGPUContext::instance().depthView());
        renderer_->endFrame();
        WGPUContext::instance().processEvents();
    }

    RenderData& renderData() { return renderer_->getRenderData(0); }
    const RenderData& renderData() const { return renderer_->getRenderData(0); }
    RenderBenchScene& scene() { return scene_; }
    const RenderBenchScene& scene() const { return scene_; }
    size_t sceneArg() const { return sceneArg_; }
    void setCounters(benchmark::State& state) const { state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(scene_.workItemCount)); }

    std::unique_ptr<Renderer3D> renderer_;
    wgpu::raii::Texture renderTarget_;
    wgpu::raii::TextureView renderTargetView_;
    RenderBenchScene scene_{};
    size_t sceneArg_ = 0;

private:
    static void initHeadlessContext() {
        static bool initialized = false;
        if (!initialized) {
            WGPUContext::instance().initHeadless(kTargetWidth, kTargetHeight);
            initialized = true;
        }
    }

    void createRenderTarget() {
        wgpu::TextureDescriptor desc{};
        desc.label = wgpu::StringView("Benchmark Render Target");
        desc.size = {kTargetWidth, kTargetHeight, 1};
        desc.format = WGPUContext::instance().surfaceFormat();
        desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.dimension = wgpu::TextureDimension::_2D;

        renderTarget_ = WGPUContext::instance().device()->createTexture(desc);
        renderTargetView_ = renderTarget_->createView();
    }
};
