#include "Tools.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "GUI/interface/interface.h"
#include "Engine/SimBox.h"

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
}

sf::RenderWindow* Tools::window = nullptr;
sf::View* Tools::gameView = nullptr;
std::unique_ptr<IRenderer>* Tools::renderer = nullptr;
SpatialGrid* Tools::grid = nullptr;
PickingSystem* Tools::pickingSystem = nullptr;
SimBox* Tools::box = nullptr;
AtomStorage* Tools::atomStorage = nullptr;
Tools::AtomCreator Tools::atomCreator = {};
Tools::AtomRemover Tools::atomRemover = {};
sf::Vector2i Tools::startMousePos = {};
bool Tools::isInteracting = false;
bool Tools::atomMoveFlag = false;
size_t Tools::selectedMoveAtomIndex = InvalidIndex;

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

    pickingSystem = new PickingSystem(rend->camera, *atomStorage, *box);
}

void Tools::resetInteractionState() {
    pickingSystem->clearSelection();
    Interface::countSelectedAtom = 0;
    Interface::drawToolTrip = false;
}

void Tools::onLeftPressed(sf::Vector2i mousePos) {
    if (Interface::cursorHovered || !renderer || !renderer->get()) {
        return;
    }

    std::unique_ptr<IRenderer>& rend = *renderer;

    Interface::drawToolTrip = false;
   
    pickingSystem->getOverlay().reset();
    startMousePos = mousePos;

    switch (currentMode()) {
    case Mode::AddAtom:
        tryAddAtom(mousePos, static_cast<AtomData::Type>(Interface::getSelectedAtom()));
        break;
    case Mode::RemoveAtom:
        tryRemoveAtom(mousePos);
        break;
    case Mode::Frame: {
        auto& overlay = pickingSystem->getOverlay();
        overlay.boxVisible = true;
        overlay.boxStart = mousePos;
        overlay.boxEnd = mousePos;
        break;
    }
    case Mode::Lasso: {
        auto& overlay = pickingSystem->getOverlay();
        overlay.lassoVisible = true;
        overlay.lassoPoints.push_back(mousePos);
        break;
    }
    case Mode::Cursor:
    default: {
        bool cumulative = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);    
        pickingSystem->processClick(mousePos, cumulative);
        AtomHit hit;
        if (pickingSystem->pickAtom(mousePos, 20.0f, hit)) {
            selectedMoveAtomIndex = hit.index;
            atomMoveFlag = true;
        }
        Interface::countSelectedAtom = pickingSystem->getSelectedIndices().size();
        break;
    }
    }
}

void Tools::onLeftReleased(sf::Vector2i mousePos) {
    auto& overlay = pickingSystem->getOverlay();
    bool cumulative = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);    

    if (overlay.boxVisible) {
        pickingSystem->processRect(overlay.boxStart, mousePos, cumulative);
    } 
    else if (overlay.lassoVisible) {
        pickingSystem->processLasso(overlay.lassoPoints, cumulative);
    }

    Interface::countSelectedAtom = pickingSystem->getSelectedIndices().size();

    overlay.reset();
    atomMoveFlag = false;
    selectedMoveAtomIndex = InvalidIndex;
}

void Tools::onFrame(sf::Vector2i mousePos, float deltaTime) {
    if (!renderer || !renderer->get() || !atomStorage || !pickingSystem) {
        return;
    }

    auto& overlay = pickingSystem->getOverlay();
    auto& rend = *renderer;

    if (overlay.boxVisible) {
        overlay.boxEnd = mousePos;
        overlay.boxVisible = true;
    }

    if (overlay.lassoVisible) {
        const float minStepSqr = 25.0f; // 5 пикселей — оптимально для плавности и памяти
        
        sf::Vector2f currentPos(mousePos.x, mousePos.y);
        sf::Vector2f lastPos(overlay.lassoPoints.back().x, overlay.lassoPoints.back().y);

        if (overlay.lassoPoints.empty() || (currentPos - lastPos).lengthSquared() >= minStepSqr) 
        {
            overlay.lassoPoints.emplace_back(mousePos);
            overlay.lassoVisible = true;
        }
    }

    // 3. Физика перемещения (Drag & Drop)
    if (atomMoveFlag && selectedMoveAtomIndex != InvalidIndex) {
        const Vec3f worldMouse = screenToBox(mousePos);
        const auto& selectedIndices = pickingSystem->getSelectedIndices();

        // Позиция "захваченного" атома
        const Vec3f selectedPos = atomStorage->pos(selectedMoveAtomIndex);
        const Vec3f force = (worldMouse - selectedPos) * 0.5f; 

        auto applyRawForce = [&](std::size_t idx, const Vec3f& f) {
            atomStorage->forceX(idx) += f.x;
            atomStorage->forceY(idx) += f.y;
            atomStorage->forceZ(idx) += f.z;
        };

        if (selectedIndices.contains(selectedMoveAtomIndex)) {
            for (std::size_t idx : selectedIndices) {
                // Проверка границ на всякий случай, если AtomStorage изменился
                if (idx < atomStorage->size()) {
                    applyRawForce(idx, force);
                }
            }
        }
        else {
            applyRawForce(selectedMoveAtomIndex, force);
        }
    }
}

Vec3f Tools::screenToWorld(sf::Vector2i mousePos) {
    return (*renderer)->camera.screenToWorld(mousePos);
}

Vec3f Tools::screenToBox(sf::Vector2i mousePos) {
    return screenToWorld(mousePos) - box->start;
}

sf::Vector2i Tools::worldToScreen(Vec3f pos) {
    return (*renderer)->camera.worldToScreen(pos);
}

sf::Vector2i Tools::boxToScreen(Vec3f pos) {
    return worldToScreen(pos + box->start);
}

Tools::Mode Tools::currentMode() {
    return mapPanelTool(Interface::sideToolsPanel.getSelectedTool());
}

bool Tools::isSelectionMode(Tools::Mode mode) {
    return mode == Mode::Frame || mode == Mode::Lasso;
}

bool Tools::tryAddAtom(sf::Vector2i mousePos, AtomData::Type atomType) {
    if (!box || !atomCreator || !atomStorage) {
        return false;
    }

    const Vec3f worldPos = screenToWorld(mousePos);

    if (!(box->start.x + 2 <= worldPos.x && worldPos.x <= box->end.x - 2 &&
          box->start.y + 2 <= worldPos.y && worldPos.y <= box->end.y - 2 &&
          box->start.z + 2 <= worldPos.z && worldPos.z <= box->end.z - 2)) {
        return false;
    }

    const Vec3f spawnPos = screenToBox(mousePos);
    const float atomRadius = AtomData::getProps(atomType).radius;

    for (std::size_t atomIndex = 0; atomIndex < atomStorage->size(); ++atomIndex) {
        const Vec3f atomPos = atomStorage->pos(atomIndex);
        const float radius = AtomData::getProps(atomStorage->type(atomIndex)).radius;
        if ((atomPos - spawnPos).abs() <= 2.f * (radius + atomRadius)) {
            return false;
        }
    }

    return atomCreator(spawnPos, Vec3f::Random() * 5.f, atomType, false);
}

bool Tools::tryRemoveAtom(sf::Vector2i mousePos) {
    if (!atomStorage || atomStorage->empty() || !atomRemover || !pickingSystem) {
        return false;
    }

    // Используем пикинг из системы
    AtomHit hit;
    if (!pickingSystem->pickAtom(mousePos, 10.0f, hit)) {
        return false;
    }

    const std::size_t target = hit.index;
    bool removed = false;

    const auto& selected = pickingSystem->getSelectedIndices();
    bool removeSelection = selected.contains(target);

    if (removeSelection) {
        std::vector<std::size_t> toRemove(selected.begin(), selected.end());
        std::sort(toRemove.begin(), toRemove.end(), std::greater<std::size_t>());

        for (std::size_t index : toRemove) {
            if (atomRemover(index)) {
                pickingSystem->handleAtomRemoval(index);
                removed = true;
            }
        }
    }
    else {
        if (atomRemover(target)) {
            pickingSystem->handleAtomRemoval(target);
            removed = true;
        }
    }

    Interface::countSelectedAtom = static_cast<int>(pickingSystem->getSelectedIndices().size());
    return removed;
}
