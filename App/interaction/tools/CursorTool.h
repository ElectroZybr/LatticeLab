#pragma once

#include "App/interaction/tools/ITool.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"

class SideToolsPanel;

class CursorTool final : public ITool {
public:
    CursorTool(ToolContext& context, SideToolsPanel& sideToolsPanel) noexcept;

    void onLeftPressed(glm::ivec2 mousePos) override;
    void onLeftReleased(glm::ivec2 mousePos) override;
    void onFrame(glm::ivec2 mousePos, float deltaTime) override;
    void reset() override;

private:
    static constexpr AtomStorage::AtomId InvalidAtomId = AtomStorage::InvalidAtomId;

    SideToolsPanel& sideToolsPanel_;
    AtomStorage::AtomId selectedMoveAtomId_ = InvalidAtomId;
    bool atomMoveActive_ = false;
};
