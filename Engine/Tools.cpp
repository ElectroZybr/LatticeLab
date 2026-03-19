#include "Tools.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

#include "SimBox.h"
#include "GUI/interface/interface.h"
#include "physics/Bond.h"

namespace {
Tools::Mode mapPanelTool(SideToolsPanel::Tool tool) {
    switch (tool) {
    case SideToolsPanel::Tool::Cursor:     return Tools::Mode::Cursor;
    case SideToolsPanel::Tool::Frame:      return Tools::Mode::Frame;
    case SideToolsPanel::Tool::Lasso:      return Tools::Mode::Lasso;
    case SideToolsPanel::Tool::AddAtom:    return Tools::Mode::AddAtom;
    case SideToolsPanel::Tool::RemoveAtom: return Tools::Mode::RemoveAtom;
    }
    return Tools::Mode::Cursor;
}

void removeBondsWithAtom(Atom* atom) {
    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        if (it->a == atom || it->b == atom) {
            it->detach();
            it = Bond::bonds_list.erase(it);
        } else {
            ++it;
        }
    }
}

void replaceAtomPointer(Atom* oldPtr, Atom* newPtr, std::vector<Atom>& atoms, Atom*& selectedMoveAtom) {
    if (!oldPtr || !newPtr || oldPtr == newPtr) {
        return;
    }

    if (selectedMoveAtom == oldPtr) {
        selectedMoveAtom = newPtr;
    }

    if (Tools::selected_atom_batch.erase(oldPtr) > 0) {
        Tools::selected_atom_batch.insert(newPtr);
    }

    for (Bond& bond : Bond::bonds_list) {
        if (bond.a == oldPtr) {
            bond.a = newPtr;
        }
        if (bond.b == oldPtr) {
            bond.b = newPtr;
        }
    }

    for (Atom& atom : atoms) {
        for (Atom*& bondedAtom : atom.bonds) {
            if (bondedAtom == oldPtr) {
                bondedAtom = newPtr;
            }
        }
    }
}

void rebuildGrid(SpatialGrid* grid, std::vector<Atom>& atoms) {
    if (!grid) {
        return;
    }

    for (int y = 0; y < grid->sizeY; ++y) {
        for (int x = 0; x < grid->sizeX; ++x) {
            if (auto* cell = grid->at(x, y)) {
                cell->clear();
            }
        }
    }

    for (Atom& atom : atoms) {
        const int cellX = grid->worldToCellX(atom.coords.x);
        const int cellY = grid->worldToCellY(atom.coords.y);
        grid->insert(cellX, cellY, &atom);
    }
}

bool removeAtomInternal(Atom* target, SpatialGrid* grid, std::vector<Atom>& atoms, Atom*& selectedMoveAtom) {
    if (!target || atoms.empty()) {
        return false;
    }

    const std::size_t removeIndex = static_cast<std::size_t>(target - atoms.data());
    if (removeIndex >= atoms.size()) {
        return false;
    }

    Tools::selected_atom_batch.erase(target);
    if (selectedMoveAtom == target) {
        selectedMoveAtom = nullptr;
    }

    removeBondsWithAtom(target);

    const std::size_t lastIndex = atoms.size() - 1;
    if (removeIndex != lastIndex) {
        Atom* movedOldPtr = &atoms[lastIndex];
        std::swap(atoms[removeIndex], atoms[lastIndex]);
        Atom* movedNewPtr = &atoms[removeIndex];
        replaceAtomPointer(movedOldPtr, movedNewPtr, atoms, selectedMoveAtom);
    }

    atoms.pop_back();
    rebuildGrid(grid, atoms);
    return true;
}

Vec3D randomSpawnVelocity() {
    return Vec3D(
        ((double)std::rand() / RAND_MAX - 0.5) * 5.0,
        ((double)std::rand() / RAND_MAX - 0.5) * 5.0,
        0.0
    );
}
}

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
IRenderer* Tools::render = nullptr;
SpatialGrid* Tools::grid = nullptr;
SimBox* Tools::box = nullptr;
std::unordered_set<Atom*> Tools::selected_atom_batch{};

void Tools::init(sf::RenderWindow* w, sf::View* gv, IRenderer* r, SpatialGrid* gr, SimBox* b) {
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
    sf::Vector2f world = (sf::Vector2f(mouse_pos) - sf::Vector2f(window->getSize()) / 2.f) / zoom + gameView->getCenter();
    return Vec2D(world.x, world.y);
}

Vec2D Tools::screenToBox(sf::Vector2i mouse_pos, float zoom) {
    return screenToWorld(mouse_pos, zoom) - Vec2D(box->start.x, box->start.y);
}

Tools::Mode Tools::currentMode() {
    return mapPanelTool(Interface::sideToolsPanel.getSelectedTool());
}

bool Tools::isSelectionMode(Tools::Mode mode) {
    return mode == Mode::Frame || mode == Mode::Lasso;
}

Atom* Tools::pickAtom(sf::Vector2i mouse_pos) {
    if (!render || !box || !grid) {
        return nullptr;
    }

    const float zoom = render->camera.getZoom();
    const Vec2D world = screenToWorld(mouse_pos, zoom);
    const Vec2D local(world.x - box->start.x, world.y - box->start.y);

    const int cellX = grid->worldToCellX(local.x);
    const int cellY = grid->worldToCellY(local.y);
    if (cellX < 0 || cellY < 0) {
        return nullptr;
    }

    Atom* best = nullptr;
    double bestDistSqr = std::numeric_limits<double>::max();

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            auto* cell = grid->at(cellX + dx, cellY + dy);
            if (!cell) {
                continue;
            }

            for (Atom* atom : *cell) {
                const Vec2D center(
                    atom->coords.x + atom->getProps().radius,
                    atom->coords.y + atom->getProps().radius
                );
                const Vec2D delta = center - local;
                const double distSqr = delta.sqrAbs();
                const double pickRadius = std::max(0.5, atom->getProps().radius);
                if (distSqr <= pickRadius * pickRadius && distSqr < bestDistSqr) {
                    bestDistSqr = distSqr;
                    best = atom;
                }
            }
        }
    }

    if (best) {
        return best;
    }

    if (auto* cell = grid->at(cellX, cellY); cell && !cell->empty()) {
        return *cell->begin();
    }

    return nullptr;
}

bool Tools::tryAddAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, int atomType) {
    if (!render || !box || atomType < 0) {
        return false;
    }

    const float zoom = render->camera.getZoom();
    const Vec2D world = screenToWorld(mouse_pos, zoom);
    const Vec2D local(world.x - box->start.x, world.y - box->start.y);

    const double atomRadius = Atom::getProps(atomType).radius;
    Vec3D spawnPos = Vec3D(local - atomRadius / 2.0, (box->end.z - box->start.z) * 0.5);

    bool hasNearAtom = false;
    for (Atom& atom : atoms) {
        if ((atom.coords - spawnPos).abs() <= atom.getProps().radius + atomRadius) {
            hasNearAtom = true;
            break;
        }
    }

    if (hasNearAtom) {
        return false;
    }

    atoms.emplace_back(spawnPos, randomSpawnVelocity(), atomType);
    return true;
}

bool Tools::tryRemoveAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom*& selectedMoveAtom) {
    if (!grid || atoms.empty()) {
        return false;
    }

    Atom* target = pickAtom(mouse_pos);
    if (!target) {
        return false;
    }

    bool removed = false;
    if (target->isSelect && !selected_atom_batch.empty()) {
        while (!selected_atom_batch.empty()) {
            Atom* selectedTarget = *selected_atom_batch.begin();
            if (!removeAtomInternal(selectedTarget, grid, atoms, selectedMoveAtom)) {
                selected_atom_batch.erase(selectedTarget);
                continue;
            }
            removed = true;
        }
    } else {
        removed = removeAtomInternal(target, grid, atoms, selectedMoveAtom);
    }

    Interface::countSelectedAtom = static_cast<int>(selected_atom_batch.size());
    return removed;
}
