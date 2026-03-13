#include "Tools.h"
#include "SimBox.h"

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
Renderer* Tools::render = nullptr;
SpatialGrid* Tools::grid = nullptr;
SimBox* Tools::box = nullptr;
std::unordered_set<Atom*> Tools::selected_atom_batch{};

void Tools::init(sf::RenderWindow* w, sf::View* gv, Renderer* r, SpatialGrid* gr, SimBox* b) {
    window = w;
    gameView = gv;
    render = r;
    grid = gr;
    box = b;
}

void Tools::selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    Vec2D start_world = screenToWorld(start_mouse_pos, render->camera.getZoom());
    Vec2D end_world = screenToWorld(mouse_pos, render->camera.getZoom());
    render->setSelectionFrame(start_world, end_world, render->camera.getZoom());

    Vec2D start_pos = start_world;
    Vec2D pos = end_world;
    if (box) {
        start_pos.x -= box->start.x;
        start_pos.y -= box->start.y;
        pos.x -= box->start.x;
        pos.y -= box->start.y;
    }

    double temp;

    if (start_pos.x - pos.x > 0) {
        temp = pos.x;
        pos.x = start_pos.x;
        start_pos.x = temp;
    }

    if (start_pos.y - pos.y > 0) {
        temp = pos.y;
        pos.y = start_pos.y;
        start_pos.y = temp;
    }
    
    int count = 0;
    for (Atom& atom : atoms) {
        if (atom.coords.x >= start_pos.x-0.8 && atom.coords.x <= pos.x && atom.coords.y >= start_pos.y-0.8 && atom.coords.y <= pos.y) {
            atom.isSelect = true;
            selected_atom_batch.insert(&atom);
            count++;
        }
        else {
            atom.isSelect = false;
            selected_atom_batch.erase(&atom);
        }   
    }
    Interface::drawToolTrip = true;
    Interface::countSelectedAtom = count;
}

Vec2D Tools::screenToWorld(sf::Vector2i mouse_pos, float zoom) {
    return Vec2D(((mouse_pos.x - (window->getSize().x / 2.f)) / zoom) + gameView->getCenter().x,
                 ((mouse_pos.y - (window->getSize().y / 2.f)) / zoom) + gameView->getCenter().y);
}

Vec2D Tools::screenToBox(sf::Vector2i mouse_pos, float zoom) {
    return screenToWorld(mouse_pos, zoom) - Vec2D(box->start.x, box->start.y);
}
