#pragma once

#include <cstdint>
#include <imgui.h>
#include <glm/vec2.hpp>

class SideToolsPanel {
public:
    enum class Tool : uint8_t { Cursor, Rect, Lasso, Brush, Ruler, AddAtom, RemoveAtom };
    enum class AreaShape : uint8_t { Rect, Lasso, Brush };
    enum class AreaAction : uint8_t { Select, Spawn };

    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, glm::ivec2 windowSize, class IOPanel& ioPanel, ImFont* iconFont, ImFont* textFont = nullptr);

    Tool getSelectedTool() const { return selectedTool; }
    void setSelectedTool(Tool tool);
    AreaShape areaShape() const { return areaShape_; }
    AreaAction areaAction() const { return areaAction_; }
    float brushRadius() const { return brushRadius_; }

private:
    static bool isAreaTool(Tool tool);
    void drawAreaContextPopup(float scale, ImVec2 anchorPos, class IOPanel& ioPanel);

    Tool selectedTool = Tool::Cursor;
    AreaShape areaShape_ = AreaShape::Rect;
    AreaAction areaAction_ = AreaAction::Select;
    float brushRadius_ = 12.0f;
    bool areaPopupVisible_ = true;
};
