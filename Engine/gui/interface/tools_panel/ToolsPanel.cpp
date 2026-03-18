#include "ToolsPanel.h"
#include "imgui.h"
#include "../debug_panel/DebugPanel.h"
#include "../file_dialog/FileDialogManager.h"

#define ICON_FA_FLASK   "\uf0c3"
#define ICON_FA_COG     "\uf013"
#define ICON_FA_BUG     "\uf188"

static constexpr ImGuiWindowFlags FIXED_FLAGS =
    ImGuiWindowFlags_NoMove     |
    ImGuiWindowFlags_NoResize   |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoScrollbar;

void ToolsPanel::draw(float scale, sf::RenderWindow& window,
                      DebugPanel& debug, FileDialogManager& fileDialog)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(183*scale, 65*scale));
    ImGui::Begin("Tools", nullptr, FIXED_FLAGS);

    if (ImGui::Button(ICON_FA_COG,   ImVec2(50*scale, 50*scale))) ImGui::OpenPopup("tools_popup");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FLASK, ImVec2(50*scale, 50*scale))) ImGui::OpenPopup("tools_popup");
    ImGui::SameLine();

    const bool debugVisible = debug.isVisible();
    if (debugVisible) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
    }
    if (ImGui::Button(ICON_FA_BUG, ImVec2(50*scale, 50*scale)))
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