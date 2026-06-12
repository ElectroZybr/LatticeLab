#include "App/viewport/SceneViewport.h"

#include "App/debug/DebugRuntime.h"
#include "App/interaction/ToolsManager.h"
#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "Rendering/Renderer2D.h"
#include "Rendering/Renderer3D.h"
#include "Rendering/BaseRenderer.h"

SceneViewport::SceneViewport(RendererType type, CaptureController& captureController) : captureController_(&captureController), renderer_(createRenderer(type)) {}

void SceneViewport::setScreenSize(int width, int height) {
    renderer_->camera.setScreenSize(glm::vec2(static_cast<float>(width), static_cast<float>(height)));
}

void SceneViewport::resetView() { renderer_->camera.resetView(); }

void SceneViewport::syncScene(const Lattice::Simulation& simulation) { App::Viewport::syncRendererWithSimulation(*renderer_, simulation); }

void SceneViewport::renderFrame(Lattice::Simulation& simulation, Interface& appInterface, const DebugViews& debugViews) {
    PROFILE_SCOPE("SceneViewport::renderFrame");

    UiState& uiState = appInterface.state();
    uiState.simStep = simulation.world().getSimStep();

    appInterface.update();
    if (renderer_->getRenderDataCount() > simulation.activeWorldId()) {
        const RenderData& activeRenderData = renderer_->getRenderData(simulation.activeWorldId());
        if (activeRenderData.drawVectorField || activeRenderData.drawFieldArrows || activeRenderData.drawFieldContours) {
            simulation.world().updateVectorField();
        }
    }

    if (ToolsManager::pickingSystem != nullptr) {
        App::Viewport::syncRendererWithSimulation(*renderer_, simulation, &ToolsManager::pickingSystem->getSelectedAtomIds());
    }
    else {
        App::Viewport::syncRendererWithSimulation(*renderer_, simulation);
    }

    refreshAtomDebugViews(debugViews, simulation);
    captureController_->renderFrame(*renderer_, [&]() {
        ToolsManager::overlay.draw();
        appInterface.draw(*renderer_);
    });
}

bool SceneViewport::setRendererType(RendererType type, const Lattice::Simulation& simulation) {
    if (renderer_ && renderer_->camera.getMode() == Camera::Mode::Mode2D) {
        cached2DCameraState_.valid = true;
        cached2DCameraState_.position = renderer_->camera.getPosition();
        cached2DCameraState_.zoom = renderer_->camera.getZoom();
    }

    std::unique_ptr<BaseRenderer> newRenderer = createRenderer(type);
    if (!newRenderer) {
        return false;
    }

    if (renderer_) {
        copyRenderSettings(*newRenderer, *renderer_);
        newRenderer->camera.setScreenSize(renderer_->camera.getScreenSize());
    }

    App::Viewport::syncRendererWithSimulation(*newRenderer, simulation);
    if (type == RendererType::Renderer2D && cached2DCameraState_.valid) {
        newRenderer->camera.setPosition(cached2DCameraState_.position);
        newRenderer->camera.setZoom(cached2DCameraState_.zoom);
    }
    else {
        newRenderer->camera.resetView();
    }
    renderer_ = std::move(newRenderer);
    return true;
}

std::unique_ptr<BaseRenderer> SceneViewport::createRenderer(RendererType type) {
    switch (type) {
    case RendererType::Renderer2D:
        return std::make_unique<Renderer2D>();
    case RendererType::Renderer3D:
        return std::make_unique<Renderer3D>();
    }

    return nullptr;
}

void SceneViewport::copyRenderSettings(BaseRenderer& destination, const BaseRenderer& source) {
    if (destination.getRenderDataCount() == 0 || source.getRenderDataCount() == 0) {
        return;
    }

    RenderData& target = destination.getRenderData(0);
    const RenderData& current = source.getRenderData(0);
    target.drawAtoms = current.drawAtoms;
    target.drawGrid = current.drawGrid;
    target.drawVectorField = current.drawVectorField;
    target.drawFieldArrows = current.drawFieldArrows;
    target.drawFieldContours = current.drawFieldContours;
    target.fieldAutoScale = current.fieldAutoScale;
    target.fieldPotentialScale = current.fieldPotentialScale;
    target.fieldCellSize = current.fieldCellSize;
    target.fieldSmoothing = current.fieldSmoothing;
    target.fieldContourStep = current.fieldContourStep;
    target.drawBonds = current.drawBonds;
    target.drawBox = current.drawBox;
    target.drawMemoryOrder = current.drawMemoryOrder;
    target.speedColorMode = current.speedColorMode;
    target.speedGradientMax = current.speedGradientMax;
}
