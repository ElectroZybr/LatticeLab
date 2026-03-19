#include "Engine/io/mouse/Mouse.h"

#include <limits>

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

    if (atomMoveFlag && selectedMoveAtom != nullptr) {
        float zoom  = render->camera.getZoom();
        Vec2D world = Tools::screenToBox(mouse_pos, zoom);
        Vec2D delta = Vec2D(selectedMoveAtom->coords.x, selectedMoveAtom->coords.y) - world;
        Vec3D force = delta * 30;
        for (Atom* atom : Tools::selected_atom_batch)
            atom->force -= force;
    }
}

void Mouse::onLeftPressed(sf::Vector2i mouse_pos) {
    if (Interface::cursorHovered) return;

    float zoom = render->camera.getZoom();
    Vec3D world = Tools::screenToWorld(mouse_pos, zoom);
    Vec3D local = world - box->start;

    switch (Interface::sideToolsPanel.getSelectedTool()) {
        case SideToolsPanel::Tool::AddAtom:
        {
            if (Interface::getSelectedAtom() == -1) break;
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
            break;
        }
        case SideToolsPanel::Tool::RemoveAtom:
        {
            // TODO работает как-то криво
            Atom* clicked = findAtomAt(local);
            if (clicked != nullptr) {
                std::erase_if(*atoms, [clicked](const Atom& a) { return &a == clicked; });
            }
            break;
        }
        case SideToolsPanel::Tool::Cursor:
        {
            Atom* clicked = findAtomAt(local);
            if (clicked != nullptr) {
                selectedMoveAtom = clicked;
                atomMoveFlag = true;
                if (!Tools::selected_atom_batch.contains(clicked)) {
                    Tools::selected_atom_batch.clear();
                    for (Atom& a : *atoms) a.isSelect = false;
                    Tools::selected_atom_batch.insert(clicked);
                    clicked->isSelect = true;
                }
            }
            break;
        }
        case SideToolsPanel::Tool::Lasso:
            // TODO Написать лассо
        case SideToolsPanel::Tool::Frame:
        {
            selectionFrameMoveFlag = true;
            start_mouse_pos = mouse_pos;
            Tools::selectionFrame(start_mouse_pos, mouse_pos, *atoms);
            render->showSelectionFrame(true);
            break;
        }
    }
}

void Mouse::onLeftReleased() {
    atomMoveFlag           = false;
    selectionFrameMoveFlag = false;
    render->showSelectionFrame(false);
    Interface::drawToolTrip = false;
}

Atom* Mouse::findAtomAt(Vec3D local) {
    static std::unordered_set<Atom*> result;
    result.clear();

    int cx = box->grid.worldToCellX(local.x);
    int cy = box->grid.worldToCellY(local.y);

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            auto* block = box->grid.at(cx + dx, cy + dy);
            if (block) result.insert(block->begin(), block->end());
        }
    }

    Atom* closest = nullptr;
    float minDist = std::numeric_limits<float>::max();
    for (Atom* a : result) {
        float dist = (Vec2D(a->coords) - Vec2D(local)).abs();

        float hitDist = dist - a->getProps().radius;
        if (hitDist < minDist) {
            minDist = hitDist;
            closest = a;
        }
    }
    return closest;
}