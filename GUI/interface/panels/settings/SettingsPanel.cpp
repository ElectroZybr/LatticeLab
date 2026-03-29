#include "SettingsPanel.h"
#include "imgui.h"

#include "Engine/Simulation.h"
#include "Rendering/BaseRenderer.h"

void SettingsPanel::draw(float uiScale, sf::Vector2u windowSize, Simulation& simulation, std::unique_ptr<IRenderer>& renderer) {
    float target = visible ? 1.f : 0.f;
    float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress += (target - animProgress) * std::min(step, 1.f);

    if (animProgress < 0.01f) return;

    const float panelWidth = 300.f * uiScale;
    const float topOffset = 65.f * uiScale;
    const float panelHeight = static_cast<float>(windowSize.y) - topOffset;

    const float x = -panelWidth + panelWidth * animProgress;

    ImGui::SetNextWindowPos(ImVec2(x, topOffset));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin("##SettingsPanel", nullptr, PANEL_FLAGS);

    ImGui::SeparatorText("Симуляция");

    ImGui::TextUnformatted("Гравитация");
    ImGui::SameLine();
    Vec3f gravity = simulation.forceField.getGravity();
    if (ImGui::Button("Reset##gravity", ImVec2(50.f * uiScale, 0.f))) {
        simulation.forceField.setGravity(Vec3f(0, 0, 0));
        gravity = simulation.forceField.getGravity();
    }

    float gx = gravity.x;
    float gy = gravity.y;
    float gz = gravity.z;
    bool gravityChanged = false;
    gravityChanged |= ImGui::SliderFloat("Gravity X##gravity_x", &gx, -10.0f, 10.0f, "%.2f");
    gravityChanged |= ImGui::SliderFloat("Gravity Y##gravity_y", &gy, -10.0f, 10.0f, "%.2f");
    gravityChanged |= ImGui::SliderFloat("Gravity Z##gravity_z", &gz, -10.0f, 10.0f, "%.2f");
    if (gravityChanged) {
        simulation.forceField.setGravity(Vec3f(gx, gy, gz));
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

    ImGui::SeparatorText("Список соседей");
    bool neighborListEnabled = simulation.isNeighborListEnabled();
    if (ImGui::Checkbox("NeighborList", &neighborListEnabled))
        simulation.setNeighborListEnabled(neighborListEnabled);

    int cellSize = simulation.sim_box.grid.cellSize;
    if (ImGui::SliderInt("Cell size", &cellSize, 1, 32)) {
        simulation.setSizeBox(simulation.sim_box.start, simulation.sim_box.end, cellSize);
    }

    if (simulation.isNeighborListEnabled()) {
        float cutoff = simulation.neighborList.cutoff();
        if (ImGui::SliderFloat("Cutoff NL", &cutoff, 0.5f, 20.0f, "%.2f")) {
            simulation.neighborList.setCutoff(cutoff);
        }

        float skin = simulation.neighborList.skin();
        if (ImGui::SliderFloat("Skin NL", &skin, 0.1f, 10.0f, "%.2f")) {
            simulation.neighborList.setSkin(skin);
        }
    }

    ImGui::End();
}
