#include "Engine/io/mouse/Mouse.h"

#include "Engine/Tools.h"
#include "GUI/interface/interface.h"

sf::RenderWindow*  Mouse::window           = nullptr;
IRenderer*         Mouse::render           = nullptr;
SimBox*            Mouse::box              = nullptr;
std::vector<Atom>* Mouse::atoms            = nullptr;
bool               Mouse::atomMoveFlag     = false;
bool               Mouse::selectionFrameMoveFlag = false;
Atom*              Mouse::selectedMoveAtom = nullptr;
sf::Vector2i       Mouse::start_mouse_pos  = {};

void Mouse::init(sf::RenderWindow* w, IRenderer* r, SimBox* b, std::vector<Atom>* a) {
    window = w;
    render = r;
    box    = b;
    atoms  = a;
}

void Mouse::onEvent(const sf::Event& event) {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left)
            onLeftPressed(mouse_pos);
    }

    if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left)
            onLeftReleased();
    }

    if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
        if (render->camera.isDragging) {
            sf::Vector2i currentPixelPos = sf::Mouse::getPosition(*window);
            sf::Vector2i deltaPixel = render->camera.dragStartPixelPos - currentPixelPos;
            sf::Vector2f deltaWorld = window->mapPixelToCoords(deltaPixel, *render->camera.view)
                                    - window->mapPixelToCoords(sf::Vector2i(0, 0), *render->camera.view);
            render->camera.position = render->camera.dragStartCameraPos + deltaWorld;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical)
            render->camera.zoomAt(e->delta, sf::Vector2f(e->position), *window);
    }

}

void Mouse::onFrame() {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);

    if (selectionFrameMoveFlag)
        Tools::selectionFrame(start_mouse_pos, mouse_pos, *atoms);

    if (atomMoveFlag) {
        float zoom  = render->camera.getZoom();
        Vec2D world = Tools::screenToBox(mouse_pos, zoom);
        Vec2D delta = Vec2D(selectedMoveAtom->coords.x, selectedMoveAtom->coords.y) - world;
        Vec3D force = delta * 30;
        for (Atom* atom : Tools::selected_atom_batch)
            atom->force -= force;
    }
}

void Mouse::onLeftPressed(sf::Vector2i mouse_pos) {
    float zoom = render->camera.getZoom();

    if (!Interface::cursorHovered && Interface::getSelectedAtom() != -1) {
        const Vec2D world = Tools::screenToWorld(mouse_pos, zoom);
        const Vec2D local(world.x - box->start.x, world.y - box->start.y);
        const double atomRadius = Atom::getProps(Interface::getSelectedAtom()).radius;
        const Vec3D spawnPos = Vec3D(local - atomRadius / 2.0);

        bool hasNearAtom = false;
        for (Atom& atom : *atoms) {
            if ((atom.coords - spawnPos).abs() <= atom.getProps().radius + atomRadius) {
                hasNearAtom = true;
                break;
            }
        }

        if (!hasNearAtom) {
            atoms->emplace_back(spawnPos,
                Vec3D(((double)std::rand() / RAND_MAX - 0.5) * 5,
                      ((double)std::rand() / RAND_MAX - 0.5) * 5, 0),
                Interface::getSelectedAtom());
        }
    } else {
        Vec3D world = Tools::screenToWorld(mouse_pos, zoom);
        Vec3D local = world - box->start;
        std::unordered_set<Atom*>* block = box->grid.at(
            box->grid.worldToCellX(local.x - 0.5),
            box->grid.worldToCellY(local.y - 0.5),
            box->grid.worldToCellY(local.z - 0.5)
        );

        if (block != nullptr && !block->empty() && !selectionFrameMoveFlag) {
            selectedMoveAtom = *block->begin();
            atomMoveFlag = true;
        } else if (!Interface::cursorHovered && !selectionFrameMoveFlag) {
            selectionFrameMoveFlag = true;
            start_mouse_pos = sf::Mouse::getPosition(*window);
            Tools::selectionFrame(start_mouse_pos, sf::Mouse::getPosition(*window), *atoms);
            render->showSelectionFrame(true);
        }
    }
}

void Mouse::onLeftReleased() {
    atomMoveFlag           = false;
    selectionFrameMoveFlag = false;
    render->showSelectionFrame(false);
    Interface::drawToolTrip = false;
}