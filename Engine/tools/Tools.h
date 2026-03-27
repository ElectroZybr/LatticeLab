#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "Engine/math/Vec3f.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/SpatialGrid.h"
#include "Rendering/BaseRenderer.h"

#include "Engine/tools/picking_system/PickingSystem.h"

class SimBox;

class Tools {
public:
    using AtomCreator = std::function<bool(Vec3f, Vec3f, AtomData::Type, bool)>;
    using AtomRemover = std::function<bool(std::size_t)>;

    enum class Mode : std::uint8_t {
        Cursor,
        Frame,
        Lasso,
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
    static Vec3f screenToBox(sf::Vector2i mousePos);
    static sf::Vector2i worldToScreen(Vec3f pos);
    static sf::Vector2i boxToScreen(Vec3f pos);
    static Vec3f worldToBox(Vec3f pos);
    static Vec3f boxToWorld(Vec3f pos);

    static void onLeftPressed(sf::Vector2i mousePos);
    static void onLeftReleased(sf::Vector2i mousePos);
    static void onFrame(sf::Vector2i mousePos, float deltaTime);
    static void resetInteractionState();

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);

    static PickingSystem* pickingSystem;

private:
    static constexpr std::size_t InvalidIndex = static_cast<std::size_t>(-1);

    static bool tryAddAtom(sf::Vector2i mousePos, AtomData::Type atomType);
    static bool tryRemoveAtom(sf::Vector2i mousePos);

    static sf::RenderWindow* window;
    static sf::View* gameView;
    static SpatialGrid* grid;
    static std::unique_ptr<IRenderer>* renderer;
    static SimBox* box;
    static AtomStorage* atomStorage;
    static AtomCreator atomCreator;
    static AtomRemover atomRemover;

    static sf::Vector2i startMousePos;
    static bool isInteracting;
    static size_t selectedMoveAtomIndex;
    static bool atomMoveFlag;
};
