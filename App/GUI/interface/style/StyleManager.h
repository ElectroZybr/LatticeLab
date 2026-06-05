#pragma once
#include <imgui.h>
#include <glm/vec2.hpp>

class StyleManager {
public:
    void applyCustomStyle();
    void onResize(glm::ivec2 newSize);
    float getScale() const { return scale; }

private:
    ImGuiStyle baseStyle;
    float scale = 1.0f;

    // базовый размер окна
    static constexpr int BASE_W = 1920;
    static constexpr int BASE_H = 1080;
};
