#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>

class PeriodicPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::Vector2u windowSize, int& selectedAtom);

    static int decodeAtom(int index);

private:
    float animProgress = 0.0f;
};
