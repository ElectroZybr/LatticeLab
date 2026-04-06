#include "ToolsManager.h"

#include <utility>

#include "App/interaction/tools/AddAtomTool.h"
#include "App/interaction/tools/CursorTool.h"
#include "App/interaction/tools/FrameTool.h"
#include "App/interaction/tools/LassoTool.h"
#include "App/interaction/tools/RemoveAtomTool.h"
#include "App/interaction/tools/RulerTool.h"
#include "Engine/SimBox.h"
#include "GUI/interface/interface.h"

namespace {
    ToolsManager::Mode mapPanelTool(SideToolsPanel::Tool tool) {
        switch (tool) {
        case SideToolsPanel::Tool::Cursor:
            return ToolsManager::Mode::Cursor;
        case SideToolsPanel::Tool::Frame:
            return ToolsManager::Mode::Frame;
        case SideToolsPanel::Tool::Lasso:
            return ToolsManager::Mode::Lasso;
        case SideToolsPanel::Tool::Ruler:
            return ToolsManager::Mode::Ruler;
        case SideToolsPanel::Tool::AddAtom:
            return ToolsManager::Mode::AddAtom;
        case SideToolsPanel::Tool::RemoveAtom:
            return ToolsManager::Mode::RemoveAtom;
        }
        return ToolsManager::Mode::Cursor;
    }
}

sf::RenderWindow* ToolsManager::window = nullptr;
sf::View* ToolsManager::gameView = nullptr;
std::unique_ptr<IRenderer>* ToolsManager::renderer = nullptr;
SpatialGrid* ToolsManager::grid = nullptr;
PickingSystem* ToolsManager::pickingSystem = nullptr;
SimBox* ToolsManager::box = nullptr;
AtomStorage* ToolsManager::atomStorage = nullptr;
ToolsManager::AtomCreator ToolsManager::atomCreator = {};
ToolsManager::AtomRemover ToolsManager::atomRemover = {};
ToolContext ToolsManager::toolContext = {};
std::array<std::unique_ptr<ITool>, ToolsManager::kModeCount> ToolsManager::toolInstances = {};
ToolsManager::Mode ToolsManager::syncedMode = ToolsManager::Mode::Cursor;
sf::Vector2i ToolsManager::startMousePos = {};
sf::Vector2i ToolsManager::lastSceneMousePos = {};
bool ToolsManager::isInteracting = false;

void ToolsManager::init(sf::RenderWindow* w, sf::View* gv, SpatialGrid* gr, SimBox* b, std::unique_ptr<IRenderer>& rend,
                        AtomStorage* storage, AtomCreator createAtomFn, AtomRemover removeAtomFn) {
    window = w;
    gameView = gv;
    grid = gr;
    box = b;
    renderer = &rend;
    atomStorage = storage;
    atomCreator = std::move(createAtomFn);
    atomRemover = std::move(removeAtomFn);

    delete pickingSystem;
    pickingSystem = new PickingSystem(*atomStorage, *box, rend);

    toolContext.window = w;
    toolContext.gameView = gv;
    toolContext.grid = gr;
    toolContext.box = b;
    toolContext.renderer = &rend;
    toolContext.atomStorage = storage;
    toolContext.pickingSystem = pickingSystem;
    toolContext.atomCreator = atomCreator;
    toolContext.atomRemover = atomRemover;

    for (auto& tool : toolInstances) {
        tool.reset();
    }
    toolInstances[toIndex(Mode::Cursor)] = std::make_unique<CursorTool>(toolContext);
    toolInstances[toIndex(Mode::Frame)] = std::make_unique<FrameTool>(toolContext);
    toolInstances[toIndex(Mode::Lasso)] = std::make_unique<LassoTool>(toolContext);
    toolInstances[toIndex(Mode::Ruler)] = std::make_unique<RulerTool>(toolContext);
    toolInstances[toIndex(Mode::AddAtom)] = std::make_unique<AddAtomTool>(toolContext);
    toolInstances[toIndex(Mode::RemoveAtom)] = std::make_unique<RemoveAtomTool>(toolContext);
    syncedMode = currentMode();
    isInteracting = false;
    lastSceneMousePos = {};
}

void ToolsManager::resetInteractionState() {
    for (auto& tool : toolInstances) {
        if (tool) {
            tool->reset();
        }
    }

    if (pickingSystem) {
        pickingSystem->clearSelection();
        pickingSystem->getOverlay().reset();
    }

    Interface::countSelectedAtom = 0;
    Interface::drawToolTrip = false;
    Interface::toolTooltipText.clear();
    isInteracting = false;
}

bool ToolsManager::isInteractingNow() noexcept { return isInteracting; }

void ToolsManager::onLeftPressed(sf::Vector2i mousePos) {
    if (Interface::cursorHovered || !renderer || !renderer->get() || !pickingSystem) {
        return;
    }

    syncToolMode();
    startMousePos = mousePos;
    lastSceneMousePos = mousePos;
    isInteracting = true;

    if (ITool* tool = activeTool(); tool != nullptr) {
        tool->onLeftPressed(mousePos);
    }
}

void ToolsManager::onLeftReleased(sf::Vector2i mousePos) {
    syncToolMode();
    if (!isInteracting) {
        return;
    }

    const sf::Vector2i releasePos = Interface::cursorHovered ? lastSceneMousePos : mousePos;

    if (ITool* tool = activeTool(); tool != nullptr) {
        tool->onLeftReleased(releasePos);
    }
    isInteracting = false;
}

bool ToolsManager::onRightPressed(sf::Vector2i mousePos) {
    if (Interface::cursorHovered) {
        return false;
    }
    syncToolMode();

    if (ITool* tool = activeTool(); tool != nullptr) {
        return tool->onRightPressed(mousePos);
    }
    return false;
}

void ToolsManager::onFrame(sf::Vector2i mousePos, float deltaTime) {
    if (!renderer || !renderer->get() || !atomStorage || !pickingSystem) {
        return;
    }

    syncToolMode();
    if (Interface::cursorHovered) {
        return;
    }

    lastSceneMousePos = mousePos;

    if (ITool* tool = activeTool(); tool != nullptr) {
        tool->onFrame(mousePos, deltaTime);
    }

    if (currentMode() == Mode::Cursor) {
        Interface::drawToolTrip = false;
        Interface::toolTooltipText.clear();
    }
}

Vec3f ToolsManager::screenToWorld(sf::Vector2i mousePos) { return (*renderer)->camera.screenToWorld(mousePos); }

sf::Vector2i ToolsManager::worldToScreen(Vec3f pos) { return (*renderer)->camera.worldToScreen(pos); }

ToolsManager::Mode ToolsManager::currentMode() { return mapPanelTool(Interface::sideToolsPanel.getSelectedTool()); }

bool ToolsManager::isSelectionMode(ToolsManager::Mode mode) { return mode == Mode::Frame || mode == Mode::Lasso; }

ITool* ToolsManager::activeTool() noexcept { return toolInstances[toIndex(currentMode())].get(); }

void ToolsManager::syncToolMode() noexcept {
    const Mode mode = currentMode();
    if (mode == syncedMode) {
        return;
    }

    if (toolInstances[toIndex(syncedMode)]) {
        toolInstances[toIndex(syncedMode)]->reset();
    }
    if (pickingSystem) {
        pickingSystem->getOverlay().reset();
    }
    Interface::drawToolTrip = false;
    Interface::toolTooltipText.clear();
    isInteracting = false;
    syncedMode = mode;
}

size_t ToolsManager::toIndex(Mode mode) noexcept { return static_cast<size_t>(mode); }
