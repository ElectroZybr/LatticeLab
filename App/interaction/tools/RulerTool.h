#pragma once

#include "App/interaction/tools/ITool.h"

class RulerTool final : public ITool {
public:
    explicit RulerTool(ToolContext& context) noexcept;

    void onLeftPressed(sf::Vector2i mousePos) override;
    void onLeftReleased(sf::Vector2i mousePos) override;
    bool onRightPressed(sf::Vector2i mousePos) override;
    void onFrame(sf::Vector2i mousePos, float deltaTime) override;
    void reset() override;

private:
    void clearMeasurement();
    void updateMeasurement(sf::Vector2i mousePos);
    void syncOverlayFromWorld();

    bool dragging_ = false;
    bool hasMeasurement_ = false;
    Vec3f startWorld_{};
    Vec3f endWorld_{};
};
