#pragma once

#include "imgui.h"
#include <SFML/System/Vector2.hpp>

class SideToolsPanel {
public:
    enum class Tool : int {
        Cursor = 0,
        Bond = 1,
        Erase = 2,
        Measure = 3,
    };

    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::Vector2u windowSize, ImFont* iconFont, ImFont* textFont = nullptr);

    Tool getSelectedTool() const { return selectedTool; }
    void setSelectedTool(Tool tool) { selectedTool = tool; }

private:
    Tool selectedTool = Tool::Cursor;
};
