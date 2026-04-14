#pragma once

#include "App/interaction/tools/ITool.h"

class RulerTool final : public ITool {
public:
    explicit RulerTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2u mousePos) override;
    void onLeftReleased(Vec2u mousePos) override;
    bool onRightPressed(Vec2u mousePos) override;
    void onFrame(Vec2u mousePos, float deltaTime) override;
    void reset() override;

private:
    void clearMeasurement();
    void updateMeasurement(Vec2u mousePos);
    void syncOverlayFromWorld();

    bool dragging_ = false;
    bool hasMeasurement_ = false;
    Vec3f startWorld_{};
    Vec3f endWorld_{};
};
