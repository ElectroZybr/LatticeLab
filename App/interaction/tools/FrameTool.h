#pragma once

#include "App/interaction/tools/ITool.h"

class FrameTool final : public ITool {
public:
    explicit FrameTool(ToolContext& context) noexcept;

    void onLeftPressed(sf::Vector2i mousePos) override;
    void onLeftReleased(sf::Vector2i mousePos) override;
    void onFrame(sf::Vector2i mousePos, float deltaTime) override;
    void reset() override;
};
