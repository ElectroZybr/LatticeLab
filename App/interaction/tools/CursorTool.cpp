#include "CursorTool.h"

#include "App/interaction/picking/PickingSystem.h"
#include "Lattice/Engine/Simulation.h"
#include "GUI/interface/UiState.h"
#include "GUI/io/keyboard/Keyboard.h"

CursorTool::CursorTool(ToolContext& context) noexcept : ITool(context) {}

void CursorTool::onLeftPressed(Vec2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr || ctx.simulation == nullptr) {
        return;
    }

    const bool cumulative = Keyboard::isPressed(GLFW_KEY_LEFT_CONTROL) || Keyboard::isPressed(GLFW_KEY_RIGHT_CONTROL);

    ctx.pickingSystem->processClick(mousePos, cumulative);

    AtomHit hit;
    if (ctx.pickingSystem->pickAtom(mousePos, 20.0f, hit)) {
        selectedMoveAtomId_ = hit.id;
        atomMoveActive_ = true;
    }

    if (ctx.uiState != nullptr) {
        ctx.uiState->selectedAtomCount = static_cast<int>(ctx.pickingSystem->getSelectedAtomIds().size());
    }
}

void CursorTool::onLeftReleased(Vec2i mousePos) {
    (void)mousePos;
    atomMoveActive_ = false;
    selectedMoveAtomId_ = InvalidAtomId;
}

void CursorTool::onFrame(Vec2i mousePos, float deltaTime) {
    ToolContext& ctx = context();
    if (!atomMoveActive_ || ctx.simulation == nullptr || ctx.pickingSystem == nullptr) {
        return;
    }
    if (deltaTime <= 0.0f) {
        return;
    }
    AtomStorage& atoms = ctx.simulation->atoms();
    const size_t selectedMoveAtomIndex = atoms.indexOf(selectedMoveAtomId_);
    if (selectedMoveAtomId_ == InvalidAtomId || selectedMoveAtomIndex >= atoms.size()) {
        atomMoveActive_ = false;
        selectedMoveAtomId_ = InvalidAtomId;
        return;
    }

    const Vec3f worldMouse = screenToLocalWorld(mousePos);
    const auto& selectedAtomIds = ctx.pickingSystem->getSelectedAtomIds();
    const Vec3f selectedWorldPos = atoms.pos(selectedMoveAtomIndex);
    const Vec3f displacement = worldMouse - selectedWorldPos;

    constexpr float kReferenceFrameRate = 60.0f;
    constexpr float kDragStrength = 5.f;
    const float frameScale = deltaTime * kReferenceFrameRate;

    auto applyRawForce = [&](size_t idx, const Vec3f& baseDisplacement) {
        const Vec3f dragForce = baseDisplacement * (kDragStrength * frameScale);
        atoms.forceX(idx) += dragForce.x;
        atoms.forceY(idx) += dragForce.y;
        atoms.forceZ(idx) += dragForce.z;
    };

    if (selectedAtomIds.contains(selectedMoveAtomId_)) {
        for (const AtomStorage::AtomId atomId : selectedAtomIds) {
            const size_t idx = atoms.indexOf(atomId);
            if (idx < atoms.size()) {
                applyRawForce(idx, displacement);
            }
        }
        return;
    }

    applyRawForce(selectedMoveAtomIndex, displacement);
}

void CursorTool::reset() {
    atomMoveActive_ = false;
    selectedMoveAtomId_ = InvalidAtomId;
}
