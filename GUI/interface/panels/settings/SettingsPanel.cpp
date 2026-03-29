#include "SettingsPanel.h"
#include "imgui.h"

#include "Engine/Simulation.h"
#include "Rendering/BaseRenderer.h"

namespace {
const char* integratorName(Integrator::Scheme scheme) {
    switch (scheme) {
        case Integrator::Scheme::Verlet: return "Velocity Verlet";
        case Integrator::Scheme::KDK: return "KDK";
        case Integrator::Scheme::RK4: return "Runge-Kutta 4";
        case Integrator::Scheme::Langevin: return "Langevin";
    }
    return "Unknown";
}
} // namespace

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

    Integrator::Scheme currentIntegrator = simulation.getIntegrator();
    if (ImGui::BeginCombo("Integrator", integratorName(currentIntegrator))) {
        const Integrator::Scheme schemes[] = {
            Integrator::Scheme::Verlet,
            Integrator::Scheme::KDK,
            Integrator::Scheme::RK4,
            Integrator::Scheme::Langevin,
        };

        for (Integrator::Scheme scheme : schemes) {
            const bool isSelected = (scheme == currentIntegrator);
            if (ImGui::Selectable(integratorName(scheme), isSelected)) {
                simulation.setIntegrator(scheme);
                currentIntegrator = scheme;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SeparatorText("Рендер");
    ImGui::Checkbox("Сетка", &renderer->drawGrid);
    ImGui::Checkbox("Связи", &renderer->drawBonds);
    ImGui::Checkbox("Градиент скорости", &renderer->speedGradient);
    ImGui::Checkbox("Турбо градиент скорости", &renderer->speedGradientTurbo);
    ImGui::TextUnformatted("Макс. скорость градиента");

    static float manualSpeedGradientMax = 5.0f;
    bool autoSpeedGradient = renderer->speedGradientMax <= 0.0f;
    if (!autoSpeedGradient) {
        manualSpeedGradientMax = renderer->speedGradientMax;
    }

    ImGui::PushItemWidth(180.0f * uiScale);
    ImGui::BeginDisabled(autoSpeedGradient);
    if (ImGui::SliderFloat("##speed_gradient_max", &manualSpeedGradientMax, 0.1f, 10.0f, "%.2f")) {
        renderer->speedGradientMax = manualSpeedGradientMax;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Checkbox("Авто##speed_gradient_auto", &autoSpeedGradient)) {
        renderer->speedGradientMax = autoSpeedGradient ? 0.0f : manualSpeedGradientMax;
    }
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

    const float exitButtonWidth = 90.0f * uiScale;
    const float footerHeight = ImGui::GetFrameHeightWithSpacing();
    const float remaining = ImGui::GetContentRegionAvail().y - footerHeight;
    if (remaining > 0.0f) {
        ImGui::Dummy(ImVec2(0.0f, remaining));
    }
    if (ImGui::Button("Exit", ImVec2(exitButtonWidth, 0.0f))) {
        pendingResult_ = SettingsCommand::ExitApplication;
    }

    ImGui::End();
}

std::optional<SettingsCommand> SettingsPanel::popResult() {
    auto result = pendingResult_;
    pendingResult_ = std::nullopt;
    return result;
}
