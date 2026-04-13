#pragma once
#include <imgui.h>

#include "Engine/math/Vec2.h"

class StyleManager {
public:
    void applyCustomStyle();
    void onResize(Vec2u newSize);
    float getScale() const { return scale; }

private:
    ImGuiStyle baseStyle;
    float scale = 1.0f;

    static constexpr int BASE_W = 800;
    static constexpr int BASE_H = 600;
};
