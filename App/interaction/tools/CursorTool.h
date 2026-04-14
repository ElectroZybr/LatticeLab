#pragma once

#include "App/interaction/tools/ITool.h"

class CursorTool final : public ITool {
public:
    explicit CursorTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2u mousePos) override;
    void onLeftReleased(Vec2u mousePos) override;
    void onFrame(Vec2u mousePos, float deltaTime) override;
    void reset() override;

private:
    static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

    size_t selectedMoveAtomIndex_ = InvalidIndex;
    bool atomMoveActive_ = false;
};
