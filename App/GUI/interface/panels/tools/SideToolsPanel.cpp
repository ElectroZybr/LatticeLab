#include "SideToolsPanel.h"

#include <array>
#include <cfloat>
#include <cmath>

#include "App/localization/i18n.h"
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

using i18n::operator""_tr;

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
        {SideToolsPanel::Tool::Cursor, ICON_FA_MOUSE_POINTER, "tool_cursor"},
        {SideToolsPanel::Tool::Rect, ICON_FA_VECTOR_SQUARE, "tool_rect"},
        {SideToolsPanel::Tool::Lasso, ICON_FA_DRAW_POLYGON, "tool_lasso"},
        {SideToolsPanel::Tool::Brush, ICON_FA_BRUSH, "tool_brush"},
        {SideToolsPanel::Tool::Ruler, ICON_FA_RULER, "tool_ruler"},
        {SideToolsPanel::Tool::AddAtom, ICON_FA_PLUS, "tool_add_atom"},
        {SideToolsPanel::Tool::RemoveAtom, ICON_FA_MINUS, "tool_remove_atom"},
    }};

    std::string_view areaShapeLabel(SideToolsPanel::AreaShape shape) {
        switch (shape) {
        case SideToolsPanel::AreaShape::Rect:
            return "tool_rect"_tr;
        case SideToolsPanel::AreaShape::Lasso:
            return "tool_lasso"_tr;
        case SideToolsPanel::AreaShape::Brush:
            return "tool_brush"_tr;
        }
        return "tool_rect"_tr;
    }

    std::string_view selectedAreaToolTitle(SideToolsPanel::Tool tool) {
        switch (tool) {
        case SideToolsPanel::Tool::Rect:
            return "tool_rect_title"_tr;
        case SideToolsPanel::Tool::Lasso:
            return "tool_lasso_title"_tr;
        case SideToolsPanel::Tool::Brush:
            return "tool_brush_title"_tr;
        default:
            return "tool_area_title"_tr;
        }
    }

    std::string_view areaActionLabel(SideToolsPanel::AreaAction action) {
        switch (action) {
        case SideToolsPanel::AreaAction::Select:
            return "tool_select"_tr;
        case SideToolsPanel::AreaAction::Spawn:
            return "tool_spawn"_tr;
        }
        return "tool_select"_tr;
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
            ImGui::TextUnformatted(i18n::tr(tooltip).data());
            if (textFont) {
                ImGui::PopFont();
            }
            ImGui::EndTooltip();
            ImGui::PopStyleVar(2);
        }

        return pressed;
    }

    bool isPointInsideRect(const ImVec2& point, const ImVec2& min, const ImVec2& max) {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
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

bool SideToolsPanel::hasContextPopup(Tool tool) {
    return tool == Tool::Cursor || isAreaTool(tool);
}

void SideToolsPanel::setSelectedTool(Tool tool) {
    if (selectedTool == tool) {
        if (hasContextPopup(tool)) {
            toolPopupVisible_ = !toolPopupVisible_;
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
        toolPopupVisible_ = true;
    }
    else if (tool == Tool::Cursor) {
        toolPopupVisible_ = true;
    }
    else {
        toolPopupVisible_ = false;
    }
}

void SideToolsPanel::updatePopupBounds() {
    popupMin_ = ImGui::GetWindowPos();
    const ImVec2 popupSize = ImGui::GetWindowSize();
    popupMax_ = ImVec2(popupMin_.x + popupSize.x, popupMin_.y + popupSize.y);
    popupBoundsValid_ = true;
}

void SideToolsPanel::drawCursorContextPopup(float scale, ImVec2 anchorPos) {
    const float popupWidth = kAreaPopupWidth * scale;

    ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popupWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 10.0f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.35f, 0.44f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.13f, 0.17f, 0.96f));

    if (ImGui::Begin("##cursor_tool_context", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        updatePopupBounds();
        ImGui::TextUnformatted("tool_cursor_title"_tr.data());
        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        ImGui::SliderFloat("##cursor_drag_strength", &cursorDragStrength_, 0.2f, 20.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("tool_strength"_tr.data());
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
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
        updatePopupBounds();
        ImGui::TextUnformatted(selectedAreaToolTitle(selectedTool).data());

        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        if (ImGui::BeginCombo("##area_action", areaActionLabel(areaAction_).data())) {
            constexpr std::array<AreaAction, 2> kActions = {
                AreaAction::Select,
                AreaAction::Spawn,
            };
            for (AreaAction action : kActions) {
                const bool selected = areaAction_ == action;
                if (ImGui::Selectable(areaActionLabel(action).data(), selected)) {
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
        ImGui::TextUnformatted("tool_mode"_tr.data());

        if (areaShape_ == AreaShape::Brush) {
            ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
            ImGui::SliderFloat("##brush_radius", &brushRadius_, 1.0f, 80.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("tool_radius"_tr.data());
        }

        if (areaAction_ == AreaAction::Spawn && ioPanel.canSpawnFromRegionTool()) {
            ImGui::SeparatorText("tool_spawn"_tr.data());
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
    const ImVec2 panelMin = ImGui::GetWindowPos();
    const ImVec2 panelSize = ImGui::GetWindowSize();
    const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);

    if (iconFont) {
        ImGui::PushFont(iconFont);
    }

    ImVec2 selectedToolMin(0.0f, 0.0f);
    bool selectedToolVisible = false;
    const bool suppressTooltips = toolPopupVisible_ && hasContextPopup(selectedTool);
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

    popupBoundsValid_ = false;
    if (toolPopupVisible_ && selectedToolVisible && hasContextPopup(selectedTool)) {
        constexpr float kPopupWidth = kAreaPopupWidth;
        constexpr float kPopupGap = 12.0f;
        const ImVec2 popupPos(selectedToolMin.x - (kPopupWidth + kPopupGap) * scale, selectedToolMin.y);
        if (textFont) {
            ImGui::PushFont(textFont);
        }
        if (selectedTool == Tool::Cursor) {
            drawCursorContextPopup(scale, popupPos);
        }
        else {
            drawAreaContextPopup(scale, popupPos, ioPanel);
        }
        if (textFont) {
            ImGui::PopFont();
        }
    }

    if (toolPopupVisible_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool clickedPanel = isPointInsideRect(mousePos, panelMin, panelMax);
        const bool clickedPopup = popupBoundsValid_ && isPointInsideRect(mousePos, popupMin_, popupMax_);
        if (!clickedPanel && !clickedPopup) {
            toolPopupVisible_ = false;
        }
    }
}
