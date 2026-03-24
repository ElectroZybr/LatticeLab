#include "SimControlPanel.h"

#define ICON_FA_PAUSE        "\uf04c"
#define ICON_FA_PLAY         "\uf04b"
#define ICON_FA_FORWARD      "\uf04e"
#define ICON_FA_FAST_FORWARD "\uf050"

static const ImVec4 ACTIVE_COLOR = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

static void pushActiveColor() {
    ImGui::PushStyleColor(ImGuiCol_Button,        ACTIVE_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ACTIVE_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACTIVE_COLOR);
}

void SimControlPanel::draw(float scale, sf::Vector2u windowSize,
                           bool& pause, float& simulationSpeed)
{
    ImGui::SetNextWindowPos(ImVec2(windowSize.x - 122*scale, 0));
    ImGui::SetNextWindowSize(ImVec2(122*scale, 111*scale));
    ImGui::Begin("SimControl", nullptr, PANEL_FLAGS);

    const bool isFast = fastMode > 0;
    if (isFast) pushActiveColor();

    const char* fastIcon = (fastMode == 2) ? ICON_FA_FAST_FORWARD : ICON_FA_FORWARD;
    if (ImGui::Button(fastIcon, ImVec2(50*scale, 50*scale))) {
        fastMode = (fastMode + 1) % 3;
    }

    if (isFast) ImGui::PopStyleColor(3);

    ImGui::SameLine();

    if (ImGui::Button(pause ? ICON_FA_PLAY : ICON_FA_PAUSE,  ImVec2(50*scale, 50*scale))) pause = !pause;

    ImGui::PushItemWidth(106*scale);
    ImGui::SliderFloat("##Speed", &simulationSpeed, 1.f, 3000.0f, "%.1f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();

    ImGui::End();
}