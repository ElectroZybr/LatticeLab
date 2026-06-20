#include "SideToolsPanel.h"

#include <array>
#include <cfloat>
#include <cmath>

#include "GUI/interface/panels/io/ioPanel.h"

#define ICON_FA_MOUSE_POINTER "\uf245"
#define ICON_FA_VECTOR_SQUARE "\uf5cb"
#define ICON_FA_RULER "\uf545"
#define ICON_FA_PLUS "\uf067"
#define ICON_FA_MINUS "\uf068"
#define ICON_FA_DRAW_POLYGON "\uf5ee"
#define ICON_FA_BRUSH "\uf55d"
#define ICON_FA_SQUARE_FULL "\uf45c"
#define ICON_FA_CIRCLE "\uf111"

namespace {
    constexpr ImVec4 ACTIVE_COLOR = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    constexpr float kAreaPopupWidth = 224.0f;
    constexpr float kCompactFieldWidth = 126.0f;

    struct ToolItem {
        SideToolsPanel::Tool tool;
        const char* icon;
        const char* tooltip;
    };

    constexpr std::array<ToolItem, 7> TOOL_ITEMS{{
        {SideToolsPanel::Tool::Cursor, ICON_FA_MOUSE_POINTER, "Cursor"},
        {SideToolsPanel::Tool::Rect, ICON_FA_VECTOR_SQUARE, "Rect"},
        {SideToolsPanel::Tool::Lasso, ICON_FA_DRAW_POLYGON, "Lasso"},
        {SideToolsPanel::Tool::Brush, ICON_FA_BRUSH, "Brush"},
        {SideToolsPanel::Tool::Ruler, ICON_FA_RULER, "Ruler"},
        {SideToolsPanel::Tool::AddAtom, ICON_FA_PLUS, "Add atom"},
        {SideToolsPanel::Tool::RemoveAtom, ICON_FA_MINUS, "Remove atom"},
    }};

    const char* areaShapeLabel(SideToolsPanel::AreaShape shape) {
        switch (shape) {
        case SideToolsPanel::AreaShape::Rect:
            return "Rect";
        case SideToolsPanel::AreaShape::Lasso:
            return "Lasso";
        case SideToolsPanel::AreaShape::Brush:
            return "Brush";
        }
        return "Rect";
    }

    const char* selectedAreaToolTitle(SideToolsPanel::Tool tool) {
        switch (tool) {
        case SideToolsPanel::Tool::Rect:
            return "Rect tool";
        case SideToolsPanel::Tool::Lasso:
            return "Lasso tool";
        case SideToolsPanel::Tool::Brush:
            return "Brush tool";
        default:
            return "Area tool";
        }
    }

    const char* areaActionLabel(SideToolsPanel::AreaAction action) {
        switch (action) {
        case SideToolsPanel::AreaAction::Select:
            return "Select";
        case SideToolsPanel::AreaAction::Spawn:
            return "Spawn";
        }
        return "Select";
    }

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

bool SideToolsPanel::isAreaTool(Tool tool) {
    switch (tool) {
    case Tool::Rect:
    case Tool::Lasso:
    case Tool::Brush:
        return true;
    default:
        return false;
    }
}

void SideToolsPanel::setSelectedTool(Tool tool) {
    if (selectedTool == tool) {
        if (isAreaTool(tool)) {
            areaPopupVisible_ = !areaPopupVisible_;
        }
        return;
    }

    selectedTool = tool;
    if (isAreaTool(tool)) {
        switch (tool) {
        case Tool::Rect:
            areaShape_ = AreaShape::Rect;
            break;
        case Tool::Lasso:
            areaShape_ = AreaShape::Lasso;
            break;
        case Tool::Brush:
            areaShape_ = AreaShape::Brush;
            break;
        default:
            break;
        }
        areaPopupVisible_ = true;
    }
    else {
        areaPopupVisible_ = false;
    }
}

void SideToolsPanel::drawAreaContextPopup(float scale, ImVec2 anchorPos, IOPanel& ioPanel) {
    const float popupWidth = kAreaPopupWidth * scale;

    ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popupWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 10.0f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.35f, 0.44f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.13f, 0.17f, 0.96f));

    if (ImGui::Begin("##area_tool_context", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(selectedAreaToolTitle(selectedTool));

        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        if (ImGui::BeginCombo("##area_action", areaActionLabel(areaAction_))) {
            constexpr std::array<AreaAction, 2> kActions = {
                AreaAction::Select,
                AreaAction::Spawn,
            };
            for (AreaAction action : kActions) {
                const bool selected = areaAction_ == action;
                if (ImGui::Selectable(areaActionLabel(action), selected)) {
                    areaAction_ = action;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mode");

        if (areaShape_ == AreaShape::Brush) {
            ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
            ImGui::SliderFloat("##brush_radius", &brushRadius_, 1.0f, 80.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Radius");
        }

        if (areaAction_ == AreaAction::Spawn && ioPanel.canSpawnFromRegionTool()) {
            ImGui::SeparatorText("Spawn");
            ioPanel.drawRegionSpawnSettings(scale, true);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
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
    const bool suppressTooltips = areaPopupVisible_ && isAreaTool(selectedTool);
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

    if (areaPopupVisible_ && selectedToolVisible && isAreaTool(selectedTool)) {
        constexpr float kPopupWidth = kAreaPopupWidth;
        constexpr float kPopupGap = 12.0f;
        const ImVec2 popupPos(selectedToolMin.x - (kPopupWidth + kPopupGap) * scale, selectedToolMin.y);
        if (textFont) {
            ImGui::PushFont(textFont);
        }
        drawAreaContextPopup(scale, popupPos, ioPanel);
        if (textFont) {
            ImGui::PopFont();
        }
    }
}
