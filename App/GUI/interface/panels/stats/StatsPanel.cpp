#include "StatsPanel.h"

void StatsPanel::draw(float scale, glm::ivec2 windowSize) {
    const float margin = 1.0f * scale;
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(windowSize.x) - margin, static_cast<float>(windowSize.y) - margin),
                            ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f * scale, 0.0f));
    ImGui::Begin("Stats", nullptr, PANEL_FLAGS);
    ImGui::SetWindowFontScale(0.68f);
    ImGui::TextColored(ImVec4(0.78f, 0.82f, 0.88f, 0.72f), "FPS %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::PopStyleVar();
}
