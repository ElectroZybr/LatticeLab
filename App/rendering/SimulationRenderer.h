#pragma once

#include <memory>

#include "App/capture/CaptureController.h"
#include "App/debug/CreateDebugPanels.h"
#include "App/rendering/SimulationSceneSource.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/api/RendererFactory.h"

class Simulation;
class Interface;

class SimulationRenderer {
public:
    SimulationRenderer(Rendering::API::RendererKind kind, CaptureController& captureController);

    [[nodiscard]] BaseRenderer& renderer() noexcept { return *renderer_; }
    [[nodiscard]] const BaseRenderer& renderer() const noexcept { return *renderer_; }
    [[nodiscard]] std::unique_ptr<BaseRenderer>& rendererHandle() noexcept { return renderer_; }
    [[nodiscard]] const std::unique_ptr<BaseRenderer>& rendererHandle() const noexcept { return renderer_; }

    void setScreenSize(int width, int height);
    void resetView();

    void syncScene(const Simulation& simulation);
    void renderFrame(const Simulation& simulation, Interface& appInterface, const DebugViews& debugViews);

    bool setRendererKind(Rendering::API::RendererKind kind, const Simulation& simulation);

private:
    static void copyRenderSettings(BaseRenderer& destination, const BaseRenderer& source);

    CaptureController* captureController_ = nullptr;
    std::unique_ptr<BaseRenderer> renderer_;
};
