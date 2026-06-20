#include "SideToolsPanel.h"

#include <array>
#include <cmath>

#include "GUI/interface/panels/io/ioPanel.h"

#define ICON_FA_MOUSE_POINTER "\uf245"
#define ICON_FA_VECTOR_SQUARE "\uf5cb"
#define ICON_FA_DRAW_POLYGON "\uf5ee"
#define ICON_FA_RULER "\uf545"
#define ICON_FA_PLUS "\uf067"
#define ICON_FA_MINUS "\uf068"
#define ICON_FA_SQUARE "\uf0c8"
#define ICON_FA_CIRCLE "\uf111"

namespace {
    constexpr ImVec4 ACTIVE_COLOR = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

    struct ToolItem {
        SideToolsPanel::Tool tool;
        const char* icon;
        const char* tooltip;
    };

    constexpr std::array<ToolItem, 8> TOOL_ITEMS{{
        {SideToolsPanel::Tool::Cursor, ICON_FA_MOUSE_POINTER, "Cursor"},
        {SideToolsPanel::Tool::Frame, ICON_FA_VECTOR_SQUARE, "Frame select"},
        {SideToolsPanel::Tool::Lasso, ICON_FA_DRAW_POLYGON, "Lasso select"},
        {SideToolsPanel::Tool::Ruler, ICON_FA_RULER, "Ruler"},
        {SideToolsPanel::Tool::AddAtom, ICON_FA_PLUS, "Add atom"},
        {SideToolsPanel::Tool::RemoveAtom, ICON_FA_MINUS, "Remove atom"},
        {SideToolsPanel::Tool::SpawnBox, ICON_FA_SQUARE, "Spawn by box"},
        {SideToolsPanel::Tool::SpawnCircle, ICON_FA_CIRCLE, "Spawn by circle"},
    }};

    bool drawToolButton(const char* icon, const char* tooltip, bool selected, bool suppressTooltip, float buttonSize, float scale,
                        ImFont* textFont) {
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ACTIVE_COLOR);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_COLOR);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACTIVE_COLOR);
        }

        const bool pressed = ImGui::Button(icon, ImVec2(buttonSize, buttonSize));

        if (selected) {
            ImGui::PopStyleColor(3);
        }

        if (!suppressTooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            const ImVec2 itemMin = ImGui::GetItemRectMin();
            const ImVec2 itemMax = ImGui::GetItemRectMax();
            const float tooltipOffset = 10.0f * scale;
            ImGui::SetNextWindowPos(ImVec2(itemMin.x - tooltipOffset, (itemMin.y + itemMax.y) * 0.5f), ImGuiCond_Always, ImVec2(1.0f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 8.0f * scale));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::BeginTooltip();
            if (textFont) {
                ImGui::PushFont(textFont);
            }
            ImGui::SetWindowFontScale(1.12f);
            ImGui::TextUnformatted(tooltip);
            if (textFont) {
                ImGui::PopFont();
            }
            ImGui::EndTooltip();
            ImGui::PopStyleVar(2);
        }

        return pressed;
    }
}

bool SideToolsPanel::isRegionSpawnTool(Tool tool) {
    return tool == Tool::SpawnBox || tool == Tool::SpawnCircle;
}

void SideToolsPanel::setSelectedTool(Tool tool) {
    if (selectedTool == tool) {
        if (isRegionSpawnTool(tool)) {
            regionSpawnPopupVisible_ = !regionSpawnPopupVisible_;
        }
        return;
    }

    selectedTool = tool;
    regionSpawnPopupVisible_ = isRegionSpawnTool(tool);
}

void SideToolsPanel::draw(float scale, glm::ivec2 windowSize, IOPanel& ioPanel, ImFont* iconFont, ImFont* textFont) {
    constexpr float baseTopOffset = 114.0f;
    constexpr float baseRightOffset = 0.0f;
    constexpr float baseSpacing = 4.0f;
    constexpr float baseButtonSize = 44.0f;
    constexpr float basePaddingX = 6.0f;
    constexpr float basePaddingY = 6.0f;
    const float buttonCount = static_cast<float>(TOOL_ITEMS.size());

    const float buttonSize = std::round(baseButtonSize * scale);
    const float spacingY = std::round(baseSpacing * scale);
    const float panelPaddingX = std::round(basePaddingX * scale);
    const float panelPaddingY = std::round(basePaddingY * scale);
    const float rightOffset = std::round(baseRightOffset * scale);

    const float panelWidthRaw = (panelPaddingX * 2.0f) + buttonSize;
    const float panelHeightRaw = (panelPaddingY * 2.0f) + (buttonCount * buttonSize) + ((buttonCount - 1.0f) * spacingY);
    const float xRaw = static_cast<float>(windowSize.x) - panelWidthRaw - rightOffset;
    const float yRaw = baseTopOffset * scale;

    const float panelWidth = std::round(panelWidthRaw);
    const float panelHeight = std::round(panelHeightRaw);
    const float x = std::round(xRaw);
    const float y = std::round(yRaw);

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(panelPaddingX, panelPaddingY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, spacingY));
    ImGui::Begin("SideTools", nullptr, PANEL_FLAGS);

    if (iconFont) {
        ImGui::PushFont(iconFont);
    }

    ImVec2 selectedToolMin(0.0f, 0.0f);
    bool selectedToolVisible = false;
    const bool suppressTooltips = regionSpawnPopupVisible_ && isRegionSpawnTool(selectedTool);
    for (const ToolItem& item : TOOL_ITEMS) {
        if (drawToolButton(item.icon, item.tooltip, selectedTool == item.tool, suppressTooltips, buttonSize, scale, textFont)) {
            setSelectedTool(item.tool);
        }
        if (selectedTool == item.tool) {
            selectedToolMin = ImGui::GetItemRectMin();
            selectedToolVisible = true;
        }
    }

    if (iconFont) {
        ImGui::PopFont();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);

    if (regionSpawnPopupVisible_ && selectedToolVisible && isRegionSpawnTool(selectedTool) && ioPanel.canSpawnFromRegionTool()) {
        constexpr float kPopupWidth = 320.0f;
        constexpr float kPopupGap = 12.0f;
        const ImVec2 popupPos(selectedToolMin.x - (kPopupWidth + kPopupGap) * scale, selectedToolMin.y);
        if (textFont) {
            ImGui::PushFont(textFont);
        }
        ioPanel.drawRegionSpawnPopup(scale, popupPos);
        if (textFont) {
            ImGui::PopFont();
        }
    }
}
