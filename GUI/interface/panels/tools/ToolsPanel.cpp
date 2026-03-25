#include <cmath>

#include "ToolsPanel.h"

#include "GUI/interface/panels/debug/DebugPanel.h"
#include "GUI/interface/panels/settings/SettingsPanel.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"

#define ICON_FA_FLASK   "\uf0c3"
#define ICON_FA_COG     "\uf013"
#define ICON_FA_BUG     "\uf188"

void ToolsPanel::draw(float scale, sf::RenderWindow& window, DebugPanel& debug, FileDialogManager& fileDialog, SettingsPanel& settings)
{
    constexpr float baseTopOffset = 0.0f;
    constexpr float baseLeftOffset = 0.0f;
    constexpr float baseSpacing = 4.0f;
    constexpr float baseButtonSize = 50.0f;
    constexpr float basePaddingX = 6.0f;
    constexpr float basePaddingY = 6.0f;
    constexpr float buttonCount = 4.0f;

    const float buttonSize = std::round(baseButtonSize * scale);
    const float spacingX = std::round(baseSpacing * scale);
    const float panelPaddingX = std::round(basePaddingX * scale);
    const float panelPaddingY = std::round(basePaddingY * scale);

    const float panelWidth = std::round((panelPaddingX * 2.0f) + (buttonCount * buttonSize) + ((buttonCount - 1.0f) * spacingX));
    const float panelHeight = std::round((panelPaddingY * 2.0f) + buttonSize);
    const float x = std::round(baseLeftOffset * scale);
    const float y = std::round(baseTopOffset * scale);

    auto toggleButton = [&](const char* icon, auto& primary, auto& secondary) {
        const bool visible = primary.isVisible();
        if (visible) {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
        }
        if (ImGui::Button(icon, ImVec2(buttonSize, buttonSize))) {
            primary.toggle();
            if (primary.isVisible() && secondary.isVisible())
                secondary.toggle();
        }
        if (visible) ImGui::PopStyleColor(3);
    };

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(panelPaddingX, panelPaddingY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacingX, 0.0f));
    ImGui::Begin("Tools", nullptr, PANEL_FLAGS);

    toggleButton(ICON_FA_COG, settings, debug);

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FLASK, ImVec2(buttonSize, buttonSize))) ImGui::OpenPopup("tools_popup");
    ImGui::SameLine();

    if (ImGui::Button(is3D ? "3D" : "2D", ImVec2(buttonSize, buttonSize)))
    {
        is3D = !is3D;
        pendingResult = is3D ? ToolsCommand::ToggleRenderer3D : ToolsCommand::ToggleRenderer2D;
    }
    ImGui::SameLine();

    toggleButton(ICON_FA_BUG, debug, settings);

    if (ImGui::BeginPopup("tools_popup")) {
        if (ImGui::MenuItem("save"))  fileDialog.openSave();
        if (ImGui::MenuItem("load"))  fileDialog.openLoad();
        if (ImGui::MenuItem("clear")) pendingResult = ToolsCommand::ClearSimulation;
        ImGui::Separator();
        if (ImGui::MenuItem("exit"))  window.close();
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

std::optional<ToolsCommand> ToolsPanel::popResult() {
    auto result = pendingResult;
    pendingResult = std::nullopt;
    return result;
}
