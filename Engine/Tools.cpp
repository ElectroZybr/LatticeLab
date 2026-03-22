#include "Tools.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

#include "math/Consts.h"
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

    for (int z = 0; z < grid->sizeZ; ++z) {
        for (int y = 0; y < grid->sizeY; ++y) {
            for (int x = 0; x < grid->sizeX; ++x) {
                if (auto* cell = grid->at(x, y, z)) {
                    cell->clear();
                }
            }
        }
    }

    for (Atom& atom : atoms) {
        const int cellX = grid->worldToCellX(atom.coords.x);
        const int cellY = grid->worldToCellY(atom.coords.y);
        const int cellZ = grid->worldToCellZ(atom.coords.z);
        grid->insert(cellX, cellY, cellZ, &atom);
    }
}

bool removeAtomInternal(Atom* target, SpatialGrid* grid, std::vector<Atom>& atoms, Atom*& selectedMoveAtom, bool rebuildAfterRemove) {
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
    if (rebuildAfterRemove) {
        rebuildGrid(grid, atoms);
    }
    return true;
}

bool pointOnSegment(const Vec2D& p, const Vec2D& a, const Vec2D& b) {
    const Vec2D ab = b - a;
    const Vec2D ap = p - a;
    const double cross = ab.x * ap.y - ab.y * ap.x;
    if (std::abs(cross) > Consts::Epsilon) {
        return false;
    }

    const double dot = ap.dot(ab);
    if (dot < 0.0) {
        return false;
    }

    return dot <= ab.sqrAbs();
}

bool isPointInsidePolygon(const Vec2D& p, const std::vector<Vec2D>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    std::size_t j = polygon.size() - 1;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vec2D& a = polygon[i];
        const Vec2D& b = polygon[j];

        if (pointOnSegment(p, a, b)) {
            return true;
        }

        const bool intersects = ((a.y > p.y) != (b.y > p.y))
            && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x);

        if (intersects) {
            inside = !inside;
        }

        j = i;
    }

    return inside;
}

void syncLassoContour(std::unique_ptr<IRenderer>& render, const SimBox* box, const std::vector<Vec2D>& localPoints) {
    if (!render.get() || !box) {
        return;
    }

    std::vector<Vec2D> worldPoints;
    worldPoints.reserve(localPoints.size());
    const Vec2D boxOffset(box->start.x, box->start.y);
    for (const Vec2D& point : localPoints) {
        worldPoints.push_back(point + boxOffset);
    }

    render->setLassoContour(worldPoints, render->camera.getZoom());
}

Atom* pickSelectedAtomWithPadding(sf::Vector2i mouse_pos, float zoom, const SimBox* box) {
    if (!box || Tools::selected_atom_batch.empty()) {
        return nullptr;
    }

    const Vec2D world = Tools::screenToWorld(mouse_pos, zoom);
    const Vec2D local(world.x - box->start.x, world.y - box->start.y);

    Atom* best = nullptr;
    double bestDistSqr = std::numeric_limits<double>::max();

    const float selectedCount = static_cast<float>(Tools::selected_atom_batch.size());
    const float extraPixels = 10.0f + std::min(20.0f, std::log2(selectedCount + 1.0f) * 6.0f);
    const double extraWorld = static_cast<double>(extraPixels / std::max(zoom, 1.0f));

    for (Atom* atom : Tools::selected_atom_batch) {
        if (!atom) {
            continue;
        }

        const Vec2D center(
            atom->coords.x + atom->getProps().radius,
            atom->coords.y + atom->getProps().radius
        );
        const Vec2D delta = center - local;
        const double distSqr = delta.sqrAbs();
        const double pickRadius = std::max(0.5, atom->getProps().radius) + extraWorld;
        if (distSqr <= pickRadius * pickRadius && distSqr < bestDistSqr) {
            bestDistSqr = distSqr;
            best = atom;
        }
    }

    return best;
}
}

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
std::unique_ptr<IRenderer>* Tools::renderer = nullptr;
SpatialGrid* Tools::grid = nullptr;
SimBox* Tools::box = nullptr;
Tools::AtomCreator Tools::atomCreator = {};
std::unordered_set<Atom*> Tools::selected_atom_batch{};
bool Tools::atomMoveFlag = false;
bool Tools::selectionFrameMoveFlag = false;
bool Tools::lassoSelectionMoveFlag = false;
Atom* Tools::selectedMoveAtom = nullptr;
sf::Vector2i Tools::start_mouse_pos = {};
std::vector<Vec2D> Tools::lassoPoints{};

void Tools::init(sf::RenderWindow* w, sf::View* gv, SpatialGrid* gr, SimBox* b, std::unique_ptr<IRenderer>& rend, AtomCreator createAtomFn) {
    window = w;
    gameView = gv;
    grid = gr;
    box = b;
    renderer = &rend;
    atomCreator = std::move(createAtomFn);
}

void Tools::onLeftPressed(sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    if (Interface::cursorHovered || !renderer->get()) {
        return;
    }

    std::unique_ptr<IRenderer>& rend = *renderer;

    atomMoveFlag = false;
    selectionFrameMoveFlag = false;
    lassoSelectionMoveFlag = false;
    Interface::drawToolTrip = false;
    rend->showSelectionFrame(false);
    rend->showLassoContour(false);
    rend->setLassoContour({}, rend->camera.getZoom());

    const auto beginFrameSelection = [&]() {
        selectionFrameMoveFlag = true;
        start_mouse_pos = mouse_pos;
        selectionFrame(start_mouse_pos, mouse_pos, atoms);
        rend->showSelectionFrame(true);
    };

    switch (currentMode()) {
    case Mode::AddAtom:
        tryAddAtom(mouse_pos, atoms, static_cast<Atom::Type>(Interface::getSelectedAtom()));
        break;
    case Mode::RemoveAtom:
        tryRemoveAtom(mouse_pos, atoms, selectedMoveAtom);
        break;
    case Mode::Frame:
        beginFrameSelection();
        break;
    case Mode::Lasso: {
        lassoSelectionMoveFlag = true;
        lassoPoints.clear();
        start_mouse_pos = mouse_pos;
        lassoPoints.push_back(screenToBox(mouse_pos, rend->camera.getZoom()));
        syncLassoContour(rend, box, lassoPoints);
        rend->showLassoContour(true);
        break;
    }
    case Mode::Cursor:
    default:
        Atom* pickedAtom = pickAtom(mouse_pos);
        if (!pickedAtom && selected_atom_batch.size() > 1) {
            pickedAtom = pickSelectedAtomWithPadding(mouse_pos, rend->camera.getZoom(), box);
        }

        if (pickedAtom) {
            selectedMoveAtom = pickedAtom;
            atomMoveFlag = true;

            if (!selected_atom_batch.contains(pickedAtom)) {
                selected_atom_batch.clear();
                for (Atom& atom : atoms) {
                    atom.isSelect = false;
                }
                pickedAtom->isSelect = true;
                selected_atom_batch.insert(pickedAtom);
                Interface::countSelectedAtom = 1;
            }
        }
        break;
    }
}

void Tools::onLeftReleased(std::vector<Atom>& atoms) {
    std::unique_ptr<IRenderer>& rend = *renderer;

    if (lassoSelectionMoveFlag && window) {
        const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
        const float zoom = rend->camera.getZoom();
        const Vec2D local = screenToBox(mouse_pos, zoom);
        if (lassoPoints.empty() || (lassoPoints.back() - local).sqrAbs() > Consts::Epsilon) {
            lassoPoints.push_back(local);
        }

        int count = 0;
        if (lassoPoints.size() >= 3) {
            selected_atom_batch.clear();
            for (Atom& atom : atoms) {
                const Vec2D atomCenter(
                    atom.coords.x + atom.getProps().radius,
                    atom.coords.y + atom.getProps().radius
                );
                const bool selected = isPointInsidePolygon(atomCenter, lassoPoints);
                atom.isSelect = selected;
                if (selected) {
                    selected_atom_batch.insert(&atom);
                    ++count;
                }
            }
        }

        Interface::countSelectedAtom = count;
        lassoPoints.clear();
        rend->setLassoContour({}, rend->camera.getZoom());
    }

    atomMoveFlag = false;
    selectionFrameMoveFlag = false;
    lassoSelectionMoveFlag = false;
    selectedMoveAtom = nullptr;

    rend->showSelectionFrame(false);
    rend->showLassoContour(false);
    rend->setLassoContour({}, rend->camera.getZoom());
    Interface::drawToolTrip = false;
}

void Tools::onFrame(std::vector<Atom>& atoms) {
    std::unique_ptr<IRenderer>& rend = *renderer;
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);

    if (selectionFrameMoveFlag) {
        selectionFrame(start_mouse_pos, mouse_pos, atoms);
    }

    if (lassoSelectionMoveFlag) {
        const float zoom = rend->camera.getZoom();
        const Vec2D local = screenToBox(mouse_pos, zoom);
        const float minWorldStep = 4.0f / std::max(zoom, 1.0f);
        const double minWorldStepSqr = static_cast<double>(minWorldStep * minWorldStep);

        if (lassoPoints.empty() || (lassoPoints.back() - local).sqrAbs() >= minWorldStepSqr) {
            lassoPoints.push_back(local);
        }

        std::vector<Vec2D> contourPoints = lassoPoints;
        if (contourPoints.empty() || (contourPoints.back() - local).sqrAbs() > Consts::Epsilon) {
            contourPoints.push_back(local);
        }
        syncLassoContour(rend, box, contourPoints);
    }

    if (atomMoveFlag && selectedMoveAtom != nullptr) {
        const float zoom = rend->camera.getZoom();
        const Vec2D world = screenToBox(mouse_pos, zoom);
        const Vec2D delta = Vec2D(selectedMoveAtom->coords.x, selectedMoveAtom->coords.y) - world;
        const Vec3D force = delta * 30;
        if (!selected_atom_batch.empty()) {
            for (Atom* atom : selected_atom_batch) {
                atom->force -= force;
            }
        } else {
            selectedMoveAtom->force -= force;
        }
    }
}

void Tools::selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    std::unique_ptr<IRenderer>& rend = *renderer;

    Vec2D start_world = screenToWorld(start_mouse_pos, rend->camera.getZoom());
    Vec2D end_world = screenToWorld(mouse_pos, rend->camera.getZoom());
    rend->setSelectionFrame(start_world, end_world, rend->camera.getZoom());

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
    if (!box || !grid) {
        return nullptr;
    }

    std::unique_ptr<IRenderer>& rend = *renderer;

    const float zoom = rend->camera.getZoom();
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
            grid->forEachAtXY(cellX + dx, cellY + dy, [&](Atom* atom) {
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
            });
        }
    }

    return best;
}

bool Tools::tryAddAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom::Type atomType) {
    if (!box || !atomCreator) {
        return false;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;

    const float zoom = rend->camera.getZoom();
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

    return atomCreator(spawnPos, Vec3D::Random() * 5.f, atomType, false) != nullptr;
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
            if (!removeAtomInternal(selectedTarget, grid, atoms, selectedMoveAtom, false)) {
                selected_atom_batch.erase(selectedTarget);
                continue;
            }
            removed = true;
        }
        if (removed) {
            rebuildGrid(grid, atoms);
        }
    } else {
        removed = removeAtomInternal(target, grid, atoms, selectedMoveAtom, true);
    }

    Interface::countSelectedAtom = static_cast<int>(selected_atom_batch.size());
    return removed;
}
