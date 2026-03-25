#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <unordered_set>
#include <memory>
#include <cstdint>

#include "physics/SpatialGrid.h"
#include "math/Vec2f.h"
#include "math/Vec3f.h"
#include "Rendering/BaseRenderer.h"

class SimBox;
class AtomStorage;

class Tools {
public:
    using AtomCreator = std::function<bool(Vec3f, Vec3f, Atom::Type, bool)>;

    enum class Mode: uint8_t {
        Cursor,
        Frame,
        Lasso,
        AddAtom,
        RemoveAtom,
    };

    static void init(sf::RenderWindow* window, sf::View* gameView, SpatialGrid* grid, SimBox* box,
                     std::unique_ptr<IRenderer>& r, AtomStorage* atomStorage = nullptr, AtomCreator atomCreator = {});

    static void selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static Vec2f screenToWorld(sf::Vector2i mouse_pos);
    static Vec2f screenToBox(sf::Vector2i mouse_pos);

    static sf::Vector2i worldToScreen(Vec2f pos);
    static sf::Vector2i boxToScreen(Vec2f pos);

    static void onLeftPressed(sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static void onLeftReleased(std::vector<Atom>& atoms);
    static void onFrame(std::vector<Atom>& atoms, float deltaTime);
    static void resetInteractionState();

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);
    static Atom* pickAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static bool tryAddAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom::Type atomType);
    static bool tryRemoveAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom*& selectedMoveAtom);

    static std::unordered_set<Atom*> selected_atom_batch;
private:
    static sf::RenderWindow* window;
    static sf::View* gameView;
    static SpatialGrid* grid;
    static std::unique_ptr<IRenderer>* renderer;
    static SimBox* box;
    static AtomStorage* atomStorage;
    static AtomCreator atomCreator;

    static bool atomMoveFlag;
    static bool selectionFrameMoveFlag;
    static bool lassoSelectionMoveFlag;
    static Atom* selectedMoveAtom;
    static sf::Vector2i start_mouse_pos;
    static std::vector<sf::Vector2i> lassoPoints;
};
