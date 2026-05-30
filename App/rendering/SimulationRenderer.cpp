#include "App/rendering/SimulationRenderer.h"

#include <imgui_impl_wgpu.h>

#include "App/debug/DebugRuntime.h"
#include "App/interaction/ToolsManager.h"
#include "Engine/Simulation.h"
#include "Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/WGPUContext.h"

SimulationRenderer::SimulationRenderer(Rendering::API::RendererKind kind, CaptureController& captureController)
    : captureController_(&captureController), renderer_(Rendering::API::createRenderer(kind, WGPUContext::instance().surfaceFormat())) {}

void SimulationRenderer::setScreenSize(int width, int height) {
    renderer_->camera.setScreenSize(glm::vec2(static_cast<float>(width), static_cast<float>(height)));
}

void SimulationRenderer::resetView() { renderer_->camera.resetView(); }

void SimulationRenderer::syncScene(const Simulation& simulation) { App::Rendering::syncRendererWithSimulation(*renderer_, simulation); }

void SimulationRenderer::renderFrame(const Simulation& simulation, Interface& appInterface, const DebugViews& debugViews) {
    PROFILE_SCOPE("SimulationRenderer::renderFrame");

    UiState& uiState = appInterface.state();
    uiState.simStep = simulation.world().getSimStep();

    if (ToolsManager::pickingSystem != nullptr) {
        App::Rendering::syncRendererWithSimulation(*renderer_, simulation, &ToolsManager::pickingSystem->getSelectedIndices());
    }
    else {
        App::Rendering::syncRendererWithSimulation(*renderer_, simulation);
    }

    appInterface.update();
    refreshAtomDebugViews(debugViews, simulation);

    WGPUContext& ctx = WGPUContext::instance();

    wgpu::SurfaceTexture surfaceTex;
    ctx.surface()->getCurrentTexture(&surfaceTex);
    wgpu::raii::Texture surfaceTexture(surfaceTex.texture);
    wgpu::raii::TextureView surfaceView = surfaceTexture->createView();

    wgpu::TextureView renderTarget = captureController_->acquireRenderTarget(*surfaceTexture, *surfaceView);

    renderer_->drawShot(renderTarget, *ctx.depthView());

    ToolsManager::overlay.draw();

    ImGui::Render();
    if (wgpu::raii::RenderPassEncoder* currentPass = renderer_->currentRenderPass(); currentPass != nullptr) {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), **currentPass);
    }

    renderer_->endFrame();

    captureController_->onFrameRendered(*surfaceTexture);

    ctx.present();
    ctx.processEvents();
}

bool SimulationRenderer::setRendererKind(Rendering::API::RendererKind kind, const Simulation& simulation) {
    std::unique_ptr<BaseRenderer> newRenderer = Rendering::API::createRenderer(kind, WGPUContext::instance().surfaceFormat());
    if (!newRenderer) {
        return false;
    }

    if (renderer_) {
        copyRenderSettings(*newRenderer, *renderer_);
        newRenderer->camera.setScreenSize(renderer_->camera.getScreenSize());
    }

    App::Rendering::syncRendererWithSimulation(*newRenderer, simulation);
    newRenderer->camera.resetView();
    renderer_ = std::move(newRenderer);
    return true;
}

void SimulationRenderer::copyRenderSettings(BaseRenderer& destination, const BaseRenderer& source) {
    if (destination.getRenderDataCount() == 0 || source.getRenderDataCount() == 0) {
        return;
    }

    RenderData& target = destination.getRenderData(0);
    const RenderData& current = source.getRenderData(0);
    target.drawGrid = current.drawGrid;
    target.drawBonds = current.drawBonds;
    target.drawBox = current.drawBox;
    target.speedColorMode = current.speedColorMode;
    target.speedGradientMax = current.speedGradientMax;
}
