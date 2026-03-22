#pragma once

#include "imgui.h"
#include <SFML/System/Vector2.hpp>

class SideToolsPanel {
public:
    enum class Tool : uint8_t {
        Cursor = 0,
        Frame = 1,
        Lasso = 2,
        AddAtom = 3,
        RemoveAtom = 4,
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
