#pragma once

#include "App/interaction/tools/ITool.h"

class LassoTool final : public ITool {
public:
    explicit LassoTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2u mousePos) override;
    void onLeftReleased(Vec2u mousePos) override;
    void onFrame(Vec2u mousePos, float deltaTime) override;
    void reset() override;
};
