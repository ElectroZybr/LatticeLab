#pragma once

#include "App/interaction/tools/ITool.h"

class FrameTool final : public ITool {
public:
    explicit FrameTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2i mousePos) override;
    void onLeftReleased(Vec2i mousePos) override;
    void onFrame(Vec2i mousePos, float deltaTime) override;
    void reset() override;
};
