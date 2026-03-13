#include <SFML/Graphics.hpp>
#include <vector>

#include "physics/SpatialGrid.h"
#include "../interface.h"
#include "math/Vec2D.h"
#include "Renderer.h"

class SimBox;

class Tools {
public:
    static void init(sf::RenderWindow* window, sf::View* gameView, Renderer* render, SpatialGrid* grid, SimBox* box);
    static void selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms);
    static Vec2D screenToWorld(sf::Vector2i mouse_pos, float zoom);
    static Vec2D screenToBox(sf::Vector2i mouse_pos, float zoom);
    static std::unordered_set<Atom*> selected_atom_batch;
private:
    static sf::RenderWindow* window;
    static sf::View* gameView;
    static SpatialGrid* grid;
    static Renderer* render;
    static SimBox* box;
};
