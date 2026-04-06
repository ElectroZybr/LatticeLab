#pragma once

#include <SFML/System/Vector2.hpp>
#include <imgui.h>

class SideToolsPanel {
public:
    enum class Tool : uint8_t { Cursor, Frame, Lasso, Ruler, AddAtom, RemoveAtom };

    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::Vector2u windowSize, ImFont* iconFont, ImFont* textFont = nullptr);

    Tool getSelectedTool() const { return selectedTool; }
    void setSelectedTool(Tool tool) { selectedTool = tool; }

private:
    Tool selectedTool = Tool::Cursor;
};
