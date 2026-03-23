#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <unordered_set>
#include <memory>
#include <cstdint>

#include "physics/SpatialGrid.h"
#include "math/Vec2D.h"
#include "math/Vec3D.h"
#include "Rendering/BaseRenderer.h"

class SimBox;
class Atom;
class AtomStorage;

class Tools {
public:
    using AtomCreator = std::function<Atom*(Vec3D, Vec3D, Atom::Type, bool)>;

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
    static Vec2D screenToWorld(sf::Vector2i mouse_pos);
    static Vec2D screenToBox(sf::Vector2i mouse_pos);

    static sf::Vector2i worldToScreen(Vec2D pos);
    static sf::Vector2i boxToScreen(Vec2D pos);

    static void onLeftPressed(sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static void onLeftReleased(std::vector<Atom>& atoms);
    static void onFrame(std::vector<Atom>& atoms, float deltaTime);

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);
    static Atom* pickAtom(sf::Vector2i mouse_pos);
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
