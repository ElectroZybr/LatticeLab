#include "ToolsPanel.h"
#include "GUI/interface/panels/debug/DebugPanel.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"

#define ICON_FA_FLASK   "\uf0c3"
#define ICON_FA_COG     "\uf013"
#define ICON_FA_BUG     "\uf188"

void ToolsPanel::draw(float scale, sf::RenderWindow& window,
                      DebugPanel& debug, FileDialogManager& fileDialog)
{
    constexpr float sizeButton = 50.f;
    constexpr int countButton = 4;
    constexpr float padding = 3.f;
    constexpr float windowW = (sizeButton + padding) * countButton + padding;
    constexpr float windowH = sizeButton + 2.f * padding;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(windowW * scale, windowH * scale));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding * scale, padding * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(padding * scale, 0.f));
    ImGui::Begin("Tools", nullptr, PANEL_FLAGS);
    ImGui::PopStyleVar(2);

    if (ImGui::Button(ICON_FA_COG,   ImVec2(sizeButton * scale, sizeButton * scale))) ImGui::OpenPopup("tools_popup");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FLASK, ImVec2(sizeButton * scale, sizeButton * scale))) ImGui::OpenPopup("tools_popup");
    ImGui::SameLine();

    if (ImGui::Button(is3D ? "3D" : "2D", ImVec2(sizeButton * scale, sizeButton * scale)))
    {
        is3D = !is3D;
        pendingResult = is3D ? ToolsCommand::ToggleRenderer3D : ToolsCommand::ToggleRenderer2D;
    }
    ImGui::SameLine();

    const bool debugVisible = debug.isVisible();
    if (debugVisible) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
    }
    if (ImGui::Button(ICON_FA_BUG, ImVec2(sizeButton * scale, sizeButton * scale)))
        debug.toggle();
    if (debugVisible)
        ImGui::PopStyleColor(3);

    if (ImGui::BeginPopup("tools_popup")) {
        if (ImGui::MenuItem("save"))  fileDialog.openSave();
        if (ImGui::MenuItem("load"))  fileDialog.openLoad();
        ImGui::Separator();
        if (ImGui::MenuItem("exit"))  window.close();
        ImGui::EndPopup();
    }

    ImGui::End();
}

std::optional<ToolsCommand> ToolsPanel::popResult() {
    auto result = pendingResult;
    pendingResult = std::nullopt;
    return result;
}
