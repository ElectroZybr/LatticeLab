#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include "App/interaction/tools/ITool.h"
#include "App/interaction/picking/PickingSystem.h"
#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/math/Vec3f.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Rendering/BaseRenderer.h"

class SimBox;

class ToolsManager {
public:
    using AtomCreator = std::function<bool(Vec3f, Vec3f, AtomData::Type, bool)>;
    using AtomRemover = std::function<bool(size_t)>;

    enum class Mode : uint8_t {
        Cursor,
        Frame,
        Lasso,
        Ruler,
        AddAtom,
        RemoveAtom,
    };

    static void init(sf::RenderWindow* window,
                     sf::View* gameView,
                     SpatialGrid* grid,
                     SimBox* box,
                     std::unique_ptr<IRenderer>& renderer,
                     AtomStorage* atomStorage = nullptr,
                     AtomCreator atomCreator = {},
                     AtomRemover atomRemover = {});

    static Vec3f screenToWorld(sf::Vector2i mousePos);
    static sf::Vector2i worldToScreen(Vec3f pos);

    static void onLeftPressed(sf::Vector2i mousePos);
    static void onLeftReleased(sf::Vector2i mousePos);
    static bool onRightPressed(sf::Vector2i mousePos);
    static void onFrame(sf::Vector2i mousePos, float deltaTime);
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

    static sf::RenderWindow* window;
    static sf::View* gameView;
    static SpatialGrid* grid;
    static std::unique_ptr<IRenderer>* renderer;
    static SimBox* box;
    static AtomStorage* atomStorage;
    static AtomCreator atomCreator;
    static AtomRemover atomRemover;
    static ToolContext toolContext;
    static std::array<std::unique_ptr<ITool>, kModeCount> toolInstances;
    static Mode syncedMode;

    static sf::Vector2i startMousePos;
    static sf::Vector2i lastSceneMousePos;
    static bool isInteracting;
};
