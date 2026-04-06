#pragma once
#include "imgui.h"

#include <SFML/Graphics.hpp>

class PeriodicPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::Vector2u windowSize, int& selectedAtom);

    static int decodeAtom(int index);

private:
    float animProgress = 0.0f;
};