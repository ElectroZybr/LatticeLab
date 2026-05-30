#pragma once

#include <memory>

#include <GLFW/glfw3.h>

#include "Signals/Signals.h"

class Simulation;
class CaptureController;
class SceneViewport;
struct UiState;

namespace AppActions {
    class Handler : public Signals::Trackable {
    public:
        Handler(GLFWwindow* window, CaptureController& captureController, Simulation& simulation, SceneViewport& renderer, UiState& uiState);

    private:
        void trackIOPanel(CaptureController& captureController, UiState& uiState, Simulation& simulation, SceneViewport& renderer);
        void trackToolsPanel(Simulation& simulation, SceneViewport& renderer);
        void trackSettingsPanel(GLFWwindow* window);
        void trackKeyboard(Simulation& simulation);
        void trackSimControlPanel(Simulation& simulation);
    };
}
