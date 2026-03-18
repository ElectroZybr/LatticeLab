#pragma once
#include "imgui.h"
#include <SFML/Graphics.hpp>

class SimControlPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::Vector2u windowSize,
              bool& pause, float& simulationSpeed);

private:
    int fastMode = 0;
};