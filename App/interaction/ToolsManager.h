#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "App/interaction/picking/PickingSystem.h"
#include "App/interaction/tools/ITool.h"
#include "Engine/Simulation.h"
#include "Engine/math/Vec3.h"
#include "Rendering/BaseRenderer.h"

class SimBox;
class SideToolsPanel;
class Interface;
struct UiState;

class ToolsManager {
public:
    enum class Mode : uint8_t {
        Cursor,
        Frame,
        Lasso,
        Ruler,
        AddAtom,
        RemoveAtom,
    };

    static void init(GLFWwindow* window, Simulation& simulation, std::unique_ptr<IRenderer>& renderer, Interface& appInterface);

    static Vec3f screenToWorld(Vec2u mousePos);
    static Vec2u worldToScreen(Vec3f pos);

    static void onLeftPressed(Vec2u mousePos);
    static void onLeftReleased(Vec2u mousePos);
    static bool onRightPressed(Vec2u mousePos);
    static void onFrame(Vec2u mousePos, float deltaTime);
    static void resetInteractionState();
    static bool isInteractingNow() noexcept;

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);

    static PickingSystem* pickingSystem;

private:
    static constexpr size_t kModeCount = 6;

    static ITool* activeTool() noexcept;
    static void syncToolMode() noexcept;
    static size_t toIndex(Mode mode) noexcept;

    static GLFWwindow* window;
    static std::unique_ptr<IRenderer>* renderer;
    static Simulation* simulation;
    static UiState* uiState;
    static SideToolsPanel* sideToolsPanel;
    static ToolContext toolContext;
    static std::array<std::unique_ptr<ITool>, kModeCount> toolInstances;
    static Mode syncedMode;

    static Vec2u startMousePos;
    static Vec2u lastSceneMousePos;
    static bool isInteracting;
};
