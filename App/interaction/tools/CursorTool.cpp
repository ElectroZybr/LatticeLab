#include "CursorTool.h"

#include <SFML/Window/Keyboard.hpp>

#include "App/interaction/picking/PickingSystem.h"
#include "Engine/physics/AtomStorage.h"
#include "GUI/interface/interface.h"

CursorTool::CursorTool(ToolContext& context) noexcept
    : ITool(context) {}

void CursorTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr || ctx.atomStorage == nullptr) {
        return;
    }

    const bool cumulative = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

    ctx.pickingSystem->processClick(mousePos, cumulative);

    AtomHit hit;
    if (ctx.pickingSystem->pickAtom(mousePos, 20.0f, hit)) {
        selectedMoveAtomIndex_ = hit.index;
        atomMoveActive_ = true;
    }

    Interface::countSelectedAtom = static_cast<int>(ctx.pickingSystem->getSelectedIndices().size());
}

void CursorTool::onLeftReleased(sf::Vector2i mousePos) {
    (void)mousePos;
    atomMoveActive_ = false;
    selectedMoveAtomIndex_ = InvalidIndex;
}

void CursorTool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)deltaTime;

    ToolContext& ctx = context();
    if (!atomMoveActive_ || ctx.atomStorage == nullptr || ctx.pickingSystem == nullptr) {
        return;
    }
    if (selectedMoveAtomIndex_ == InvalidIndex || selectedMoveAtomIndex_ >= ctx.atomStorage->size()) {
        atomMoveActive_ = false;
        selectedMoveAtomIndex_ = InvalidIndex;
        return;
    }

    AtomStorage& atoms = *ctx.atomStorage;
    const Vec3f worldMouse = screenToWorld(mousePos);
    const auto& selectedIndices = ctx.pickingSystem->getSelectedIndices();
    const Vec3f selectedWorldPos = atoms.pos(selectedMoveAtomIndex_);
    const Vec3f force = (worldMouse - selectedWorldPos) * 0.05f;

    auto applyRawForce = [&](size_t idx, const Vec3f& value) {
        atoms.forceX(idx) += value.x;
        atoms.forceY(idx) += value.y;
        atoms.forceZ(idx) += value.z;
    };

    if (selectedIndices.contains(selectedMoveAtomIndex_)) {
        for (size_t idx : selectedIndices) {
            if (idx < atoms.size()) {
                applyRawForce(idx, force);
            }
        }
        return;
    }

    applyRawForce(selectedMoveAtomIndex_, force);
}

void CursorTool::reset() {
    atomMoveActive_ = false;
    selectedMoveAtomIndex_ = InvalidIndex;
}
