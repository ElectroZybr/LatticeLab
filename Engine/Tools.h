#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "math/Vec2f.h"
#include "math/Vec3f.h"
#include "physics/AtomData.h"
#include "physics/AtomStorage.h"
#include "physics/SpatialGrid.h"
#include "Rendering/BaseRenderer.h"

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

    static Vec2f screenToWorld(sf::Vector2i mousePos);
    static Vec2f screenToBox(sf::Vector2i mousePos);

    static sf::Vector2i worldToScreen(Vec2f pos);
    static sf::Vector2i boxToScreen(Vec2f pos);

    static void onLeftPressed(sf::Vector2i mousePos);
    static void onLeftReleased();
    static void onFrame(float deltaTime);
    static void resetInteractionState();

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);

    static std::unordered_set<std::size_t> selected_atom_batch;

private:
    static constexpr std::size_t InvalidIndex = static_cast<std::size_t>(-1);

    static void selectionFrame(sf::Vector2i startMousePos, sf::Vector2i mousePos);
    static std::size_t pickAtom(sf::Vector2i mousePos);
    static std::size_t pickSelectedAtomWithPadding(sf::Vector2i mousePos, float zoom);
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

    static bool atomMoveFlag;
    static bool selectionFrameMoveFlag;
    static bool lassoSelectionMoveFlag;
    static std::size_t selectedMoveAtomIndex;
    static sf::Vector2i start_mouse_pos;
    static std::vector<sf::Vector2i> lassoPoints;
};
