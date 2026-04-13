#pragma once
#include <imgui.h>

#include "Engine/math/Vec2.h"

class PeriodicPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, Vec2u windowSize, int& selectedAtom);

    static int decodeAtom(int index);

private:
    float animProgress = 0.0f;
};
