#include "SettingsPanel.h"
#include "imgui.h"

#include "Engine/Simulation.h"
#include "Rendering/BaseRenderer.h"

void SettingsPanel::draw(float uiScale, sf::Vector2u windowSize, Simulation& simulation, std::unique_ptr<IRenderer>& renderer) {
    float target = visible ? 1.f : 0.f;
    float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress += (target - animProgress) * std::min(step, 1.f);

    if (animProgress < 0.01f) return;

    const float panelWidth = 350.f * uiScale;
    const float topOffset = 65.f * uiScale;
    const float panelHeight = static_cast<float>(windowSize.y) - topOffset;

    const float x = -panelWidth + panelWidth * animProgress;

    ImGui::SetNextWindowPos(ImVec2(x, topOffset));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin("##SettingsPanel", nullptr, PANEL_FLAGS);

    ImGui::SeparatorText("Симуляция");

    Vec3f gravity = simulation.forceField.getGravity();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    if (ImGui::Button("0##gravity", ImVec2(22, 36))) {
        simulation.forceField.setGravity(Vec3f(0, 0, 0));
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();

    float buf[3] = { static_cast<float>(gravity.x), static_cast<float>(gravity.y), static_cast<float>(gravity.z) };
    if (ImGui::SliderFloat3("Гравитация", buf, -20, 20)) {
        gravity = Vec3f(buf[0], buf[1], buf[2]);
        simulation.forceField.setGravity(gravity);
    }

    ImGui::SeparatorText("Рендер");
    ImGui::Checkbox("Сетка", &renderer->drawGrid);
    ImGui::Checkbox("Связи", &renderer->drawBonds);
    ImGui::Checkbox("Градиент скорости", &renderer->speedGradient);
    ImGui::Checkbox("Турбо градиент скорости", &renderer->speedGradientTurbo);
    ImGui::TextUnformatted("Макс. скорость градиента");
    ImGui::PushItemWidth(220.0f * uiScale);
    ImGui::SliderFloat("##speed_gradient_max", &renderer->speedGradientMax, 0.0f, 10.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::End();
}
