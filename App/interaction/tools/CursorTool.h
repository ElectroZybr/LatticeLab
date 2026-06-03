#pragma once

#include "App/interaction/tools/ITool.h"
#include "Lattice/Engine/physics/AtomStorage.h"

class CursorTool final : public ITool {
public:
    explicit CursorTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2i mousePos) override;
    void onLeftReleased(Vec2i mousePos) override;
    void onFrame(Vec2i mousePos, float deltaTime) override;
    void reset() override;

private:
    static constexpr AtomStorage::AtomId InvalidAtomId = AtomStorage::InvalidAtomId;

    AtomStorage::AtomId selectedMoveAtomId_ = InvalidAtomId;
    bool atomMoveActive_ = false;
};
