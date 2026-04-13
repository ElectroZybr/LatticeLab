#pragma once

#include "App/interaction/tools/ITool.h"

class LassoTool final : public ITool {
public:
    explicit LassoTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2i mousePos) override;
    void onLeftReleased(Vec2i mousePos) override;
    void onFrame(Vec2i mousePos, float deltaTime) override;
    void reset() override;
};
