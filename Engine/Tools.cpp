#include "Tools.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

#include "GUI/interface/interface.h"
#include "SimBox.h"
#include "math/Consts.h"

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
} // namespace

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
std::unique_ptr<IRenderer>* Tools::renderer = nullptr;
SpatialGrid* Tools::grid = nullptr;
SimBox* Tools::box = nullptr;
AtomStorage* Tools::atomStorage = nullptr;
Tools::AtomCreator Tools::atomCreator = {};
Tools::AtomRemover Tools::atomRemover = {};
std::unordered_set<std::size_t> Tools::selected_atom_batch{};
bool Tools::atomMoveFlag = false;
bool Tools::selectionFrameMoveFlag = false;
bool Tools::lassoSelectionMoveFlag = false;
std::size_t Tools::selectedMoveAtomIndex = Tools::InvalidIndex;
sf::Vector2i Tools::start_mouse_pos = {};
std::vector<sf::Vector2i> Tools::lassoPoints{};

void Tools::init(sf::RenderWindow* w,
                 sf::View* gv,
                 SpatialGrid* gr,
                 SimBox* b,
                 std::unique_ptr<IRenderer>& rend,
                 AtomStorage* storage,
                 AtomCreator createAtomFn,
                 AtomRemover removeAtomFn) {
    window = w;
    gameView = gv;
    grid = gr;
    box = b;
    renderer = &rend;
    atomStorage = storage;
    atomCreator = std::move(createAtomFn);
    atomRemover = std::move(removeAtomFn);
}

void Tools::resetInteractionState() {
    if (atomStorage) {
        for (std::size_t atomIndex : selected_atom_batch) {
            if (atomIndex < atomStorage->size()) {
                atomStorage->setSelected(atomIndex, false);
            }
        }
    }
    selected_atom_batch.clear();

    selectedMoveAtomIndex = InvalidIndex;
    atomMoveFlag = false;
    selectionFrameMoveFlag = false;
    lassoSelectionMoveFlag = false;
    lassoPoints.clear();
    start_mouse_pos = {};
    Interface::countSelectedAtom = 0;
    Interface::drawToolTrip = false;

    if (renderer && renderer->get()) {
        (*renderer)->showBoxContour(false);
        (*renderer)->showLassoContour(false);
        (*renderer)->setLassoContour({});
    }
}

void Tools::onLeftPressed(sf::Vector2i mousePos) {
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
        start_mouse_pos = mousePos;
        selectionFrame(start_mouse_pos, mousePos);
        rend->showBoxContour(true);
    };

    switch (currentMode()) {
    case Mode::AddAtom:
        tryAddAtom(mousePos, static_cast<AtomData::Type>(Interface::getSelectedAtom()));
        break;
    case Mode::RemoveAtom:
        tryRemoveAtom(mousePos);
        break;
    case Mode::Frame:
        beginFrameSelection();
        break;
    case Mode::Lasso:
        lassoSelectionMoveFlag = true;
        lassoPoints.clear();
        start_mouse_pos = mousePos;
        lassoPoints.emplace_back(mousePos);
        syncLassoContour(rend, lassoPoints);
        rend->showLassoContour(true);
        break;
    case Mode::Cursor:
    default: {
        std::size_t pickedIndex = pickAtom(mousePos);
        if (pickedIndex == InvalidIndex && selected_atom_batch.size() > 1) {
            pickedIndex = pickSelectedAtomWithPadding(mousePos, rend->camera.getZoom());
        }

        if (pickedIndex != InvalidIndex) {
            selectedMoveAtomIndex = pickedIndex;
            atomMoveFlag = true;

            if (!selected_atom_batch.contains(pickedIndex)) {
                for (std::size_t atomIndex : selected_atom_batch) {
                    if (atomStorage && atomIndex < atomStorage->size()) {
                        atomStorage->setSelected(atomIndex, false);
                    }
                }
                selected_atom_batch.clear();
                selected_atom_batch.insert(pickedIndex);
                if (atomStorage && pickedIndex < atomStorage->size()) {
                    atomStorage->setSelected(pickedIndex, true);
                }
                Interface::countSelectedAtom = 1;
            }
        }
        break;
    }
    }
}

void Tools::onLeftReleased() {
    if (!renderer || !renderer->get() || !atomStorage) {
        return;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;

    if (lassoSelectionMoveFlag && window) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
        if (lassoPoints.empty() || (lassoPoints.back() - mousePos).lengthSquared() > Consts::Epsilon) {
            lassoPoints.emplace_back(mousePos);
        }

        int count = Interface::countSelectedAtom;
        if (lassoPoints.size() >= 3) {
            count = 0;
            for (std::size_t atomIndex : selected_atom_batch) {
                if (atomIndex < atomStorage->size()) {
                    atomStorage->setSelected(atomIndex, false);
                }
            }
            selected_atom_batch.clear();

            for (std::size_t atomIndex = 0; atomIndex < atomStorage->size(); ++atomIndex) {
                const Vec3f pos = atomStorage->pos(atomIndex);
                const Vec2f center(pos.x, pos.y);
                const sf::Vector2i centerScreen = boxToScreen(center);
                const bool selected = isPointInsidePolygon(centerScreen, lassoPoints);
                atomStorage->setSelected(atomIndex, selected);
                if (selected) {
                    selected_atom_batch.insert(atomIndex);
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
    selectedMoveAtomIndex = InvalidIndex;

    rend->showBoxContour(false);
    rend->showLassoContour(false);
    rend->setLassoContour({});
    Interface::drawToolTrip = false;
}

void Tools::onFrame(float deltaTime) {
    if (!renderer || !renderer->get() || !window || !atomStorage) {
        return;
    }
    std::unique_ptr<IRenderer>& rend = *renderer;
    const sf::Vector2i mousePos = sf::Mouse::getPosition(*window);

    if (selectionFrameMoveFlag) {
        selectionFrame(start_mouse_pos, mousePos);
    }

    if (lassoSelectionMoveFlag) {
        const float zoom = rend->camera.getZoom();
        const float minWorldStep = 4.0f / std::max(zoom, 1.0f);
        const double minWorldStepSqr = static_cast<double>(minWorldStep * minWorldStep);

        if (lassoPoints.empty() || (lassoPoints.back() - mousePos).lengthSquared() >= minWorldStepSqr) {
            lassoPoints.emplace_back(mousePos);
        }

        std::vector<sf::Vector2i>& contourPoints = lassoPoints;
        if (contourPoints.empty() || (contourPoints.back() - mousePos).lengthSquared() > Consts::Epsilon) {
            contourPoints.emplace_back(mousePos);
        }
        syncLassoContour(rend, contourPoints);
    }

    if (atomMoveFlag && selectedMoveAtomIndex != InvalidIndex && selectedMoveAtomIndex < atomStorage->size()) {
        (void)deltaTime;
        const Vec2f world = screenToBox(mousePos);
        const Vec3f selectedPos = atomStorage->pos(selectedMoveAtomIndex);
        const Vec2f delta = Vec2f(selectedPos.x, selectedPos.y) - world;
        constexpr float dragStiffness = 0.1f;
        const Vec3f force = delta * dragStiffness;

        if (!selected_atom_batch.empty()) {
            for (std::size_t atomIndex : selected_atom_batch) {
                if (atomIndex < atomStorage->size()) {
                    atomStorage->forceX(atomIndex) -= force.x;
                    atomStorage->forceY(atomIndex) -= force.y;
                    atomStorage->forceZ(atomIndex) -= force.z;
                }
            }
        } else {
            atomStorage->forceX(selectedMoveAtomIndex) -= force.x;
            atomStorage->forceY(selectedMoveAtomIndex) -= force.y;
            atomStorage->forceZ(selectedMoveAtomIndex) -= force.z;
        }
    }
}

void Tools::selectionFrame(sf::Vector2i startMousePos, sf::Vector2i mousePos) {
    if (!renderer || !renderer->get() || !atomStorage) {
        return;
    }

    std::unique_ptr<IRenderer>& rend = *renderer;
    rend->setBoxContour(startMousePos, mousePos);

    Vec2f sLocal = screenToBox(startMousePos);
    Vec2f eLocal = screenToBox(mousePos);

    if (sLocal.x > eLocal.x) std::swap(sLocal.x, eLocal.x);
    if (sLocal.y > eLocal.y) std::swap(sLocal.y, eLocal.y);

    for (std::size_t atomIndex : selected_atom_batch) {
        if (atomIndex < atomStorage->size()) {
            atomStorage->setSelected(atomIndex, false);
        }
    }
    selected_atom_batch.clear();

    int count = 0;
    for (std::size_t atomIndex = 0; atomIndex < atomStorage->size(); ++atomIndex) {
        const Vec3f pos = atomStorage->pos(atomIndex);
        const bool selected = (sLocal.x <= pos.x && pos.x <= eLocal.x &&
                               sLocal.y <= pos.y && pos.y <= eLocal.y);
        atomStorage->setSelected(atomIndex, selected);
        if (selected) {
            selected_atom_batch.insert(atomIndex);
            ++count;
        }
    }

    Interface::drawToolTrip = true;
    Interface::countSelectedAtom = count;
}

Vec2f Tools::screenToWorld(sf::Vector2i mousePos) {
    sf::Vector2f world = window->mapPixelToCoords(mousePos, *gameView);
    return Vec2f(world.x, world.y);
}

Vec2f Tools::screenToBox(sf::Vector2i mousePos) {
    return screenToWorld(mousePos) - Vec2f(box->start.x, box->start.y);
}

sf::Vector2i Tools::worldToScreen(Vec2f pos) {
    return window->mapCoordsToPixel(pos, *gameView);
}

sf::Vector2i Tools::boxToScreen(Vec2f pos) {
    return worldToScreen(pos + Vec2f(box->start.x, box->start.y));
}

Tools::Mode Tools::currentMode() {
    return mapPanelTool(Interface::sideToolsPanel.getSelectedTool());
}

bool Tools::isSelectionMode(Tools::Mode mode) {
    return mode == Mode::Frame || mode == Mode::Lasso;
}

std::size_t Tools::pickAtom(sf::Vector2i mousePos) {
    if (!renderer || !renderer->get() || !box || !grid || !atomStorage) {
        return InvalidIndex;
    }

    const Vec2f local = screenToBox(mousePos);
    const int cellX = grid->worldToCellX(local.x);
    const int cellY = grid->worldToCellY(local.y);
    if (cellX < 0 || cellY < 0) {
        return InvalidIndex;
    }

    std::size_t bestIndex = InvalidIndex;
    double bestDistSqr = std::numeric_limits<double>::max();

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int z = 0; z < grid->sizeZ; ++z) {
                const auto* indexCell = grid->atIndex(cellX + dx, cellY + dy, z);
                if (!indexCell) {
                    continue;
                }
                for (std::size_t atomIndex : *indexCell) {
                    if (atomIndex >= atomStorage->size()) {
                        continue;
                    }
                    const Vec3f pos = atomStorage->pos(atomIndex);
                    const float radius = std::max(0.5f, AtomData::getProps(atomStorage->type(atomIndex)).radius);
                    const Vec2f center(pos.x, pos.y);
                    const Vec2f delta = center - local;
                    const double distSqr = delta.sqrAbs();
                    if (distSqr <= radius * radius && distSqr < bestDistSqr) {
                        bestDistSqr = distSqr;
                        bestIndex = atomIndex;
                    }
                }
            }
        }
    }

    return bestIndex;
}

std::size_t Tools::pickSelectedAtomWithPadding(sf::Vector2i mousePos, float zoom) {
    if (!box || !atomStorage || selected_atom_batch.empty()) {
        return InvalidIndex;
    }

    const Vec2f world = screenToWorld(mousePos);
    const Vec2f local(world.x - box->start.x, world.y - box->start.y);

    std::size_t bestIndex = InvalidIndex;
    double bestDistSqr = std::numeric_limits<double>::max();

    const float selectedCount = static_cast<float>(selected_atom_batch.size());
    const float extraPixels = 10.0f + std::min(20.0f, std::log2(selectedCount + 1.0f) * 6.0f);
    const double extraWorld = static_cast<double>(extraPixels / std::max(zoom, 1.0f));

    for (std::size_t atomIndex : selected_atom_batch) {
        if (atomIndex >= atomStorage->size()) {
            continue;
        }
        const Vec3f pos = atomStorage->pos(atomIndex);
        const float radius = AtomData::getProps(atomStorage->type(atomIndex)).radius;
        const Vec2f center(pos.x, pos.y);
        const Vec2f delta = center - local;
        const double distSqr = delta.sqrAbs();
        const float pickRadius = std::max(0.5f, radius) + extraWorld;
        if (distSqr <= pickRadius * pickRadius && distSqr < bestDistSqr) {
            bestDistSqr = distSqr;
            bestIndex = atomIndex;
        }
    }

    return bestIndex;
}

bool Tools::tryAddAtom(sf::Vector2i mousePos, AtomData::Type atomType) {
    if (!box || !atomCreator || !atomStorage) {
        return false;
    }

    const Vec2f worldPos = screenToWorld(mousePos);

    if (!(box->start.x + 2 <= worldPos.x && worldPos.x <= box->end.x - 2 &&
          box->start.y + 2 <= worldPos.y && worldPos.y <= box->end.y - 2)) {
        return false;
    }

    const Vec2f spawnPos = screenToBox(mousePos);
    const float atomRadius = AtomData::getProps(atomType).radius;

    for (std::size_t atomIndex = 0; atomIndex < atomStorage->size(); ++atomIndex) {
        const Vec3f atomPos = atomStorage->pos(atomIndex);
        const float radius = AtomData::getProps(atomStorage->type(atomIndex)).radius;
        if ((Vec2f(atomPos.x, atomPos.y) - spawnPos).abs() <= 2.f * (radius + atomRadius)) {
            return false;
        }
    }

    return atomCreator(spawnPos, Vec3f::Random() * 5.f, atomType, false);
}

bool Tools::tryRemoveAtom(sf::Vector2i mousePos) {
    if (!grid || !atomStorage || atomStorage->empty() || !atomRemover) {
        return false;
    }

    const std::size_t target = pickAtom(mousePos);
    if (target == InvalidIndex || target >= atomStorage->size()) {
        return false;
    }

    bool removed = false;
    const bool removeSelection = atomStorage->isSelected(target) && !selected_atom_batch.empty();
    const auto removeByIndex = [&](std::size_t index) -> bool {
        if (index >= atomStorage->size()) {
            return false;
        }

        const std::size_t oldSize = atomStorage->size();
        const std::size_t movedFrom = oldSize - 1;
        if (!atomRemover(index)) {
            return false;
        }

        selected_atom_batch.erase(index);
        if (selectedMoveAtomIndex == index) {
            selectedMoveAtomIndex = InvalidIndex;
        }

        if (index < movedFrom) {
            if (selected_atom_batch.erase(movedFrom) > 0) {
                selected_atom_batch.insert(index);
            }
            if (selectedMoveAtomIndex == movedFrom) {
                selectedMoveAtomIndex = index;
            }
        }

        return true;
    };

    if (removeSelection) {
        std::vector<std::size_t> toRemove(selected_atom_batch.begin(), selected_atom_batch.end());
        std::sort(toRemove.begin(), toRemove.end(), std::greater<std::size_t>());
        for (std::size_t index : toRemove) {
            if (removeByIndex(index)) {
                removed = true;
            }
        }
    } else {
        removed = removeByIndex(target);
    }

    Interface::countSelectedAtom = static_cast<int>(selected_atom_batch.size());
    return removed;
}
