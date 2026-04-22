#pragma once

#include <memory>

#include <GLFW/glfw3.h>

#include "Signals/Signals.h"

class Simulation;
class CaptureController;
class IRenderer;
struct UiState;

namespace AppActions {
    class Handler : public Signals::Trackable {
    public:
        Handler(GLFWwindow* window, CaptureController& captureController, Simulation& simulation, std::unique_ptr<IRenderer>& renderer,
                UiState& uiState);

    private:
        void trackIOPanel(CaptureController& captureController, UiState& uiState, Simulation& simulation,
                          std::unique_ptr<IRenderer>& renderer);
        void trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer);
        void trackSettingsPanel(GLFWwindow* window);
        void trackKeyboard(Simulation& simulation);
        void trackSimControlPanel(Simulation& simulation);
    };
}
