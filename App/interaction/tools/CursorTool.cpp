#include "CursorTool.h"

#include "App/interaction/picking/PickingSystem.h"
#include "Lattice/Engine/Simulation.h"
#include "GUI/interface/UiState.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/interface/panels/tools/SideToolsPanel.h"

CursorTool::CursorTool(ToolContext& context, SideToolsPanel& sideToolsPanel) noexcept : ITool(context), sideToolsPanel_(sideToolsPanel) {}

void CursorTool::onLeftPressed(glm::ivec2 mousePos) {
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

void CursorTool::onLeftReleased(glm::ivec2 mousePos) {
    (void)mousePos;
    atomMoveActive_ = false;
    selectedMoveAtomId_ = InvalidAtomId;
}

void CursorTool::onFrame(glm::ivec2 mousePos, float deltaTime) {
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

    const glm::vec3 worldMouse = screenToLocalWorld(mousePos);
    const auto& selectedAtomIds = ctx.pickingSystem->getSelectedAtomIds();
    const glm::vec3 selectedWorldPos = atoms.pos(selectedMoveAtomIndex);
    const glm::vec3 displacement = worldMouse - selectedWorldPos;

    constexpr float kReferenceFrameRate = 60.0f;
    const float frameScale = deltaTime * kReferenceFrameRate;
    const float dragStrength = sideToolsPanel_.cursorDragStrength();

    auto applyRawForce = [&](size_t idx, const glm::vec3& baseDisplacement) {
        const glm::vec3 dragForce = baseDisplacement * (dragStrength * frameScale);
        atoms.fx()[idx] += dragForce.x;
        atoms.fy()[idx] += dragForce.y;
        atoms.fz()[idx] += dragForce.z;
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
