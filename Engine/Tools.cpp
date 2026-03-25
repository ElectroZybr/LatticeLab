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
#include "physics/AtomStorage.h"

namespace {
std::size_t atomIndexFromPtr(const Atom* atom, const std::vector<Atom>& atoms) {
    return static_cast<std::size_t>(atom - atoms.data());
}

Vec3D atomPosition(const Atom* atom, const AtomStorage* atomStorage, const std::vector<Atom>& atoms) {
    if (atomStorage && atom >= atoms.data() && atom < atoms.data() + atoms.size()) {
        return atomStorage->pos(atomIndexFromPtr(atom, atoms));
    }
    return Vec3D();
}

Vec2D atomCenter2D(const Atom* atom, const AtomStorage* atomStorage, const std::vector<Atom>& atoms) {
    const Vec3D pos = atomPosition(atom, atomStorage, atoms);
    return Vec2D(pos.x + atom->getProps().radius, pos.y + atom->getProps().radius);
}

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

void removeBondsWithAtom(Atom* atom, AtomStorage* atomStorage, std::vector<Atom>& atoms) {
    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        if (it->a == atom || it->b == atom) {
            it->detach();
            if (atomStorage) {
                if (it->a >= atoms.data() && it->a < atoms.data() + atoms.size()) {
                    const std::size_t aIndex = static_cast<std::size_t>(it->a - atoms.data());
                    atomStorage->valenceCount(aIndex) = it->a->valence;
                }
                if (it->b >= atoms.data() && it->b < atoms.data() + atoms.size()) {
                    const std::size_t bIndex = static_cast<std::size_t>(it->b - atoms.data());
                    atomStorage->valenceCount(bIndex) = it->b->valence;
                }
            }
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

void rebuildGrid(SpatialGrid* grid, const AtomStorage* atomStorage, std::vector<Atom>& atoms) {
    if (!grid) {
        return;
    }

    for (int z = 0; z < grid->sizeZ; ++z) {
        for (int y = 0; y < grid->sizeY; ++y) {
            for (int x = 0; x < grid->sizeX; ++x) {
                if (auto* cell = grid->at(x, y, z)) {
                    cell->clear();
                }
                if (auto* cell = grid->atIndex(x, y, z)) {
                    cell->clear();
                }
            }
        }
    }

    for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        Atom& atom = atoms[atomIndex];
        const Vec3D pos = atomStorage ? atomStorage->pos(atomIndex) : Vec3D();
        const int cellX = grid->worldToCellX(pos.x);
        const int cellY = grid->worldToCellY(pos.y);
        const int cellZ = grid->worldToCellZ(pos.z);
        grid->insert(cellX, cellY, cellZ, &atom);
        grid->insertIndex(cellX, cellY, cellZ, atomIndex);
    }
}

bool removeAtomInternal(Atom* target, SpatialGrid* grid, AtomStorage* atomStorage, std::vector<Atom>& atoms, Atom*& selectedMoveAtom, bool rebuildAfterRemove) {
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

    removeBondsWithAtom(target, atomStorage, atoms);

    const std::size_t lastIndex = atoms.size() - 1;
    if (removeIndex != lastIndex) {
        Atom* movedOldPtr = &atoms[lastIndex];
        std::swap(atoms[removeIndex], atoms[lastIndex]);
        Atom* movedNewPtr = &atoms[removeIndex];
        replaceAtomPointer(movedOldPtr, movedNewPtr, atoms, selectedMoveAtom);
    }

    if (atomStorage) {
        atomStorage->removeAtom(removeIndex);
    }

    atoms.pop_back();
    if (rebuildAfterRemove) {
        rebuildGrid(grid, atomStorage, atoms);
    }
    return true;
}

bool pointOnSegment(const sf::Vector2i& p, const sf::Vector2i& a, const sf::Vector2i& b) {
    const sf::Vector2i ab = b - a;
    const sf::Vector2i ap = p - a;
    const double cross = ab.x * ap.y - ab.y * ap.x;
    if (std::abs(cross) > Consts::Epsilon) {
        return false;
    }

    const double dot = ap.dot(ab);
    if (dot < 0.0) {
        return false;
    }

    return dot <= ab.lengthSquared();
}

bool isPointInsidePolygon(const sf::Vector2i& p, const std::vector<sf::Vector2i>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    int minX = polygon[0].x, maxX = polygon[0].x;
    int minY = polygon[0].y, maxY = polygon[0].y;

    for (std::size_t i = 1; i < polygon.size(); ++i) {
        if (polygon[i].x < minX) minX = polygon[i].x;
        else if (polygon[i].x > maxX) maxX = polygon[i].x;
        if (polygon[i].y < minY) minY = polygon[i].y;
        else if (polygon[i].y > maxY) maxY = polygon[i].y;
    }

    if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY) {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const sf::Vector2i& a = polygon[i];
        const sf::Vector2i& b = polygon[j];

        if (pointOnSegment(p, a, b)) {
            return true;
        }

        const bool intersects = ((a.y > p.y) != (b.y > p.y))
            && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x);

        if (intersects) {
            inside = !inside;
        }
    }

    return inside;
}

void syncLassoContour(std::unique_ptr<IRenderer>& render, const std::vector<sf::Vector2i>& screenPoints) {
    render->setLassoContour(screenPoints);
}

Atom* pickSelectedAtomWithPadding(sf::Vector2i mousePos, float zoom, const SimBox* box, const AtomStorage* atomStorage, const std::vector<Atom>& atoms) {
    if (!box || Tools::selected_atom_batch.empty()) {
        return nullptr;
    }

    const Vec2D world = Tools::screenToWorld(mousePos);
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

        const Vec2D center = atomCenter2D(atom, atomStorage, atoms);
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
} // namespace

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
std::unique_ptr<IRenderer>* Tools::renderer = nullptr;
SpatialGrid* Tools::grid = nullptr;
SimBox* Tools::box = nullptr;
AtomStorage* Tools::atomStorage = nullptr;
Tools::AtomCreator Tools::atomCreator = {};
std::unordered_set<Atom*> Tools::selected_atom_batch{};
bool Tools::atomMoveFlag = false;
bool Tools::selectionFrameMoveFlag = false;
bool Tools::lassoSelectionMoveFlag = false;
Atom* Tools::selectedMoveAtom = nullptr;
sf::Vector2i Tools::start_mouse_pos = {};
std::vector<sf::Vector2i> Tools::lassoPoints{};

void Tools::init(sf::RenderWindow* w, sf::View* gv, SpatialGrid* gr, SimBox* b,
                 std::unique_ptr<IRenderer>& rend, AtomStorage* storage, AtomCreator createAtomFn) {
    window = w;
    gameView = gv;
    grid = gr;
    box = b;
    renderer = &rend;
    atomStorage = storage;
    atomCreator = std::move(createAtomFn);
}

void Tools::onLeftPressed(sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    if (Interface::cursorHovered || !renderer || !renderer->get()) {
        return;
    }

    std::unique_ptr<IRenderer>& rend = *renderer;

    atomMoveFlag = false;
    selectionFrameMoveFlag = false;
    lassoSelectionMoveFlag = false;
    Interface::drawToolTrip = false;
    rend->showBoxContour(false);
    rend->showLassoContour(false);
    rend->setLassoContour({});

    const auto beginFrameSelection = [&]() {
        selectionFrameMoveFlag = true;
        start_mouse_pos = mouse_pos;
        selectionFrame(start_mouse_pos, mouse_pos, atoms);
        rend->showBoxContour(true);
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
    case Mode::Lasso:
        lassoSelectionMoveFlag = true;
        lassoPoints.clear();
        start_mouse_pos = mouse_pos;
        lassoPoints.emplace_back(mouse_pos);
        syncLassoContour(rend, lassoPoints);
        rend->showLassoContour(true);
        break;
    case Mode::Cursor:
    default: {
        Atom* pickedAtom = pickAtom(mouse_pos, atoms);
        if (!pickedAtom && selected_atom_batch.size() > 1) {
            pickedAtom = pickSelectedAtomWithPadding(mouse_pos, rend->camera.getZoom(), box, atomStorage, atoms);
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
}

void Tools::onLeftReleased(std::vector<Atom>& atoms) {
    if (!renderer || !renderer->get()) {
        return;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;

    if (lassoSelectionMoveFlag && window) {
        const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
        if (lassoPoints.empty() || (lassoPoints.back() - mouse_pos).lengthSquared() > Consts::Epsilon) {
            lassoPoints.emplace_back(mouse_pos);
        }

        int count = 0;
        if (lassoPoints.size() >= 3) {
            selected_atom_batch.clear();
            for (Atom& atom : atoms) {
                const Vec2D atomCenter = atomCenter2D(&atom, atomStorage, atoms);
                const sf::Vector2i atomScreenCenter = boxToScreen(atomCenter);
                const bool selected = isPointInsidePolygon(atomScreenCenter, lassoPoints);
                atom.isSelect = selected;
                if (selected) {
                    selected_atom_batch.insert(&atom);
                    ++count;
                }
            }
        }

        Interface::drawToolTrip = true;
        Interface::countSelectedAtom = count;
        lassoPoints.clear();
        rend->setLassoContour({});
    }

    atomMoveFlag = false;
    selectionFrameMoveFlag = false;
    lassoSelectionMoveFlag = false;
    selectedMoveAtom = nullptr;

    rend->showBoxContour(false);
    rend->showLassoContour(false);
    rend->setLassoContour({});
    Interface::drawToolTrip = false;
}

void Tools::onFrame(std::vector<Atom>& atoms, float deltaTime) {
    if (!renderer || !renderer->get() || !window) {
        return;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);

    if (selectionFrameMoveFlag) {
        selectionFrame(start_mouse_pos, mouse_pos, atoms);
    }

    if (lassoSelectionMoveFlag) {
        const float zoom = rend->camera.getZoom();
        const float minWorldStep = 4.0f / std::max(zoom, 1.0f);
        const double minWorldStepSqr = static_cast<double>(minWorldStep * minWorldStep);

        if (lassoPoints.empty() || (lassoPoints.back() - mouse_pos).lengthSquared() >= minWorldStepSqr) {
            lassoPoints.emplace_back(mouse_pos);
        }

        std::vector<sf::Vector2i>& contourPoints = lassoPoints;
        if (contourPoints.empty() || (contourPoints.back() - mouse_pos).lengthSquared() > Consts::Epsilon) {
            contourPoints.emplace_back(mouse_pos);
        }
        syncLassoContour(rend, contourPoints);
    }

    if (atomMoveFlag && selectedMoveAtom != nullptr) {
        const Vec2D world = screenToBox(mouse_pos);
        const Vec3D selectedPos = atomPosition(selectedMoveAtom, atomStorage, atoms);
        const Vec2D delta = Vec2D(selectedPos.x, selectedPos.y) - world;
        const Vec3D force = delta * 50.f * deltaTime;

        if (!selected_atom_batch.empty()) {
            for (Atom* atom : selected_atom_batch) {
                if (atomStorage && atom >= atoms.data() && atom < atoms.data() + atoms.size()) {
                    const std::size_t atomIndex = atomIndexFromPtr(atom, atoms);
                    atomStorage->forceX(atomIndex) -= static_cast<float>(force.x);
                    atomStorage->forceY(atomIndex) -= static_cast<float>(force.y);
                    atomStorage->forceZ(atomIndex) -= static_cast<float>(force.z);
                }
            }
        } else {
            if (atomStorage && selectedMoveAtom >= atoms.data() && selectedMoveAtom < atoms.data() + atoms.size()) {
                const std::size_t atomIndex = atomIndexFromPtr(selectedMoveAtom, atoms);
                atomStorage->forceX(atomIndex) -= static_cast<float>(force.x);
                atomStorage->forceY(atomIndex) -= static_cast<float>(force.y);
                atomStorage->forceZ(atomIndex) -= static_cast<float>(force.z);
            }
        }
    }
}

void Tools::selectionFrame(sf::Vector2i start_mouse_pos, sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    if (!renderer || !renderer->get()) {
        return;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;

    rend->setBoxContour(start_mouse_pos, mouse_pos);

    Vec2D s_local = screenToBox(start_mouse_pos);
    Vec2D e_local = screenToBox(mouse_pos);

    if (s_local.x > e_local.x) std::swap(s_local.x, e_local.x);
    if (s_local.y > e_local.y) std::swap(s_local.y, e_local.y);

    int count = 0;
    for (Atom& atom : atoms) {
        const Vec3D atomPos = atomPosition(&atom, atomStorage, atoms);
        if (s_local.x <= atomPos.x && atomPos.x <= e_local.x &&
            s_local.y <= atomPos.y && atomPos.y <= e_local.y) {
            atom.isSelect = true;
            selected_atom_batch.insert(&atom);
            ++count;
        } else {
            atom.isSelect = false;
            selected_atom_batch.erase(&atom);
        }
    }
    Interface::drawToolTrip = true;
    Interface::countSelectedAtom = count;
}

Vec2D Tools::screenToWorld(sf::Vector2i mouse_pos) {
    sf::Vector2f world = window->mapPixelToCoords(mouse_pos, *gameView);
    return Vec2D(world.x, world.y);
}

Vec2D Tools::screenToBox(sf::Vector2i mouse_pos) {
    return screenToWorld(mouse_pos) - Vec2D(box->start.x, box->start.y);
}

sf::Vector2i Tools::worldToScreen(Vec2D pos) {
    return window->mapCoordsToPixel(pos, *gameView);
}

sf::Vector2i Tools::boxToScreen(Vec2D pos) {
    return worldToScreen(pos + Vec2D(box->start.x, box->start.y));
}

Tools::Mode Tools::currentMode() {
    return mapPanelTool(Interface::sideToolsPanel.getSelectedTool());
}

bool Tools::isSelectionMode(Tools::Mode mode) {
    return mode == Mode::Frame || mode == Mode::Lasso;
}

Atom* Tools::pickAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms) {
    if (!renderer || !renderer->get() || !box || !grid) {
        return nullptr;
    }

    const Vec2D local = screenToBox(mouse_pos);
    const int cellX = grid->worldToCellX(local.x);
    const int cellY = grid->worldToCellY(local.y);
    if (cellX < 0 || cellY < 0) {
        return nullptr;
    }

    Atom* best = nullptr;
    double bestDistSqr = std::numeric_limits<double>::max();

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int z = 0; z < grid->sizeZ; ++z) {
                const auto* indexCell = grid->atIndex(cellX + dx, cellY + dy, z);
                if (!indexCell) {
                    continue;
                }
                for (std::size_t atomIndex : *indexCell) {
                    if (atomIndex >= atoms.size()) {
                        continue;
                    }

                    Atom* atom = &atoms[atomIndex];
                    const Vec2D center = atomCenter2D(atom, atomStorage, atoms);
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
    }

    return best;
}

bool Tools::tryAddAtom(sf::Vector2i mouse_pos, std::vector<Atom>& atoms, Atom::Type atomType) {
    if (!box || !atomCreator) {
        return false;
    }

    Vec2D world_pos = screenToWorld(mouse_pos);

    if (!(box->start.x + 2 <= world_pos.x && world_pos.x <= box->end.x - 2 &&
          box->start.y + 2 <= world_pos.y && world_pos.y <= box->end.y - 2)) {
        return false;
    }

    const Vec2D spawnPos = screenToBox(mouse_pos);
    const double atomRadius = Atom::getProps(atomType).radius;

    bool hasNearAtom = false;
    for (Atom& atom : atoms) {
        const Vec3D atomPos = atomPosition(&atom, atomStorage, atoms);
        if ((Vec2D(atomPos.x, atomPos.y) - spawnPos).abs() <= 2.f * (atom.getProps().radius + atomRadius)) {
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

    Atom* target = pickAtom(mouse_pos, atoms);
    if (!target) {
        return false;
    }

    bool removed = false;
    if (target->isSelect && !selected_atom_batch.empty()) {
        while (!selected_atom_batch.empty()) {
            Atom* selectedTarget = *selected_atom_batch.begin();
            if (!removeAtomInternal(selectedTarget, grid, atomStorage, atoms, selectedMoveAtom, false)) {
                selected_atom_batch.erase(selectedTarget);
                continue;
            }
            removed = true;
        }
        if (removed) {
            rebuildGrid(grid, atomStorage, atoms);
        }
    } else {
        removed = removeAtomInternal(target, grid, atomStorage, atoms, selectedMoveAtom, true);
    }

    Interface::countSelectedAtom = static_cast<int>(selected_atom_batch.size());
    return removed;
}
