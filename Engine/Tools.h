#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <unordered_set>

#include "physics/SpatialGrid.h"
#include "math/Vec2D.h"
#include "math/Vec3D.h"
#include "Rendering/BaseRenderer.h"

class SimBox;
class Atom;

class Tools {
public:
    using AtomCreator = std::function<Atom*(Vec3D, Vec3D, int, bool)>;

    enum class Mode {
        Cursor = 0,
        Frame = 1,
        Lasso = 2,
        AddAtom = 3,
        RemoveAtom = 4,
    };

    static void init(sf::RenderWindow* window, sf::View* gameView, IRenderer* render, SpatialGrid* grid, SimBox* box, AtomCreator atomCreator = {});
    static void selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static Vec2D screenToWorld(sf::Vector2i mouse_pos, float zoom);
    static Vec2D screenToBox(sf::Vector2i mouse_pos, float zoom);
    static void onLeftPressed(sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static void onLeftReleased(std::vector<Atom>& atoms);
    static void onFrame(std::vector<Atom>& atoms);

    static Mode currentMode();
    static bool isSelectionMode(Mode mode);
    static Atom* pickAtom(sf::Vector2i mouse_pos);
    static bool tryAddAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, int atomType);
    static bool tryRemoveAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom*& selectedMoveAtom);

    static std::unordered_set<Atom*> selected_atom_batch;
private:
    static sf::RenderWindow* window;
    static sf::View* gameView;
    static SpatialGrid* grid;
    static IRenderer* render;
    static SimBox* box;
    static AtomCreator atomCreator;

    static bool atomMoveFlag;
    static bool selectionFrameMoveFlag;
    static bool lassoSelectionMoveFlag;
    static Atom* selectedMoveAtom;
    static sf::Vector2i start_mouse_pos;
    static std::vector<Vec2D> lassoPoints;
};
