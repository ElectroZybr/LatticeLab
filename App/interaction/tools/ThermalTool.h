#pragma once

#include "App/interaction/tools/ITool.h"

class ThermalTool final : public ITool {
public:
    enum class Mode : unsigned char {
        Heat,
        Cool,
    };

    explicit ThermalTool(ToolContext& context) noexcept;

    void onLeftPressed(sf::Vector2i mousePos) override;
    void onLeftReleased(sf::Vector2i mousePos) override;
    void onFrame(sf::Vector2i mousePos, float deltaTime) override;
    void reset() override;

    void setMode(Mode mode) noexcept { mode_ = mode; }
    [[nodiscard]] Mode mode() const noexcept { return mode_; }

    void setRadius(float radius) noexcept;
    [[nodiscard]] float radius() const noexcept { return radius_; }

    void setStrength(float strength) noexcept;
    [[nodiscard]] float strength() const noexcept { return strength_; }

private:
    void applyAt(sf::Vector2i mousePos, float deltaTime);

    Mode mode_ = Mode::Heat;
    float radius_ = 8.0f;
    float strength_ = 1.0f;
    bool active_ = false;
};
