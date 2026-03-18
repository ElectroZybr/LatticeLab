#pragma once
#include "imgui.h"
#include <SFML/System/Vector2.hpp>

class StyleManager {
public:
    void applyCustomStyle();
    void onResize(sf::Vector2u newSize);
    float getScale() const { return scale; }

private:
    ImGuiStyle baseStyle;
    float scale = 1.0f;

    static constexpr int BASE_W = 800;
    static constexpr int BASE_H = 600;
};