#pragma once

#include "App/interaction/tools/ITool.h"

class FrameTool final : public ITool {
public:
    explicit FrameTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2u mousePos) override;
    void onLeftReleased(Vec2u mousePos) override;
    void onFrame(Vec2u mousePos, float deltaTime) override;
    void reset() override;
};
