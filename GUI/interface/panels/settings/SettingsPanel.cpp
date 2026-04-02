#include "SettingsPanel.h"
#include <imgui.h>

#include "Engine/Simulation.h"
#include "GUI/interface/style/ComboStyle.h"
#include "Rendering/BaseRenderer.h"
#include "App/AppSignals.h"

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

const char* speedColorModeName(IRenderer::SpeedColorMode mode) {
    switch (mode) {
        case IRenderer::SpeedColorMode::AtomColor: return "Обычная раскраска";
        case IRenderer::SpeedColorMode::GradientClassic: return "Градиент скорости";
        case IRenderer::SpeedColorMode::GradientTurbo: return "Турбо градиент";
    }
    return "Обычная раскраска";
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
    if (ComboStyle::beginCombo("Integrator", integratorName(currentIntegrator), 0.0f, uiScale)) {
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

    if (currentIntegrator == Integrator::Scheme::RK4 || currentIntegrator == Integrator::Scheme::Langevin) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.75f, 0.25f, 1.00f));
        ImGui::TextWrapped("Внимание: %s пока не реализован и временно работает как Velocity Verlet.",
                           integratorName(currentIntegrator));
        ImGui::PopStyleColor();
    }

    float maxParticleSpeed = simulation.getMaxParticleSpeed();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat("Скорость света", &maxParticleSpeed, 0.0f, 100.0f, maxParticleSpeed <= 0.0f ? "не ограничена" : "%.2f")) {
        simulation.setMaxParticleSpeed(maxParticleSpeed);
    }
    ImGui::PopItemWidth();

    float accelDamping = simulation.getAccelDamping();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat("Accel damping", &accelDamping, 0.0f, 1.0f, "%.3f")) {
        simulation.setAccelDamping(accelDamping);
    }
    ImGui::PopItemWidth();

    float dt = simulation.getDt();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat("Time step (Dt)", &dt, 0.0001f, 0.05f, "%.4f",
                           ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic)) {
        simulation.setDt(dt);
    }
    ImGui::PopItemWidth();
    ImGui::SeparatorText("Рендер");
    ImGui::Checkbox("Сетка", &renderer->drawGrid);
    ImGui::Checkbox("Связи", &renderer->drawBonds);

    ImGui::TextUnformatted("Цветовая схема");
    IRenderer::SpeedColorMode speedMode = renderer->speedColorMode;
    if (ComboStyle::beginCombo("##speed_color_mode", speedColorModeName(speedMode), 220.0f * uiScale, uiScale, ImGuiComboFlags_HeightLargest)) {
        const IRenderer::SpeedColorMode modes[] = {
            IRenderer::SpeedColorMode::AtomColor,
            IRenderer::SpeedColorMode::GradientClassic,
            IRenderer::SpeedColorMode::GradientTurbo,
        };

        for (IRenderer::SpeedColorMode mode : modes) {
            const bool isSelected = (mode == speedMode);
            if (ImGui::Selectable(speedColorModeName(mode), isSelected)) {
                speedMode = mode;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    renderer->speedColorMode = speedMode;

    ImGui::TextUnformatted("Макс. скорость градиента");

    static float manualSpeedGradientMax = 5.0f;
    bool autoSpeedGradient = renderer->speedGradientMax <= 0.0f;
    const bool gradientModeEnabled = renderer->speedColorMode != IRenderer::SpeedColorMode::AtomColor;
    if (!autoSpeedGradient) {
        manualSpeedGradientMax = renderer->speedGradientMax;
    }

    ImGui::PushItemWidth(180.0f * uiScale);
    ImGui::BeginDisabled(autoSpeedGradient || !gradientModeEnabled);
    if (ImGui::SliderFloat("##speed_gradient_max", &manualSpeedGradientMax, 0.1f, 10.0f, "%.2f")) {
        renderer->speedGradientMax = manualSpeedGradientMax;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!gradientModeEnabled);
    if (ImGui::Checkbox("Авто##speed_gradient_auto", &autoSpeedGradient)) {
        renderer->speedGradientMax = autoSpeedGradient ? 0.0f : manualSpeedGradientMax;
    }
    ImGui::EndDisabled();
    ImGui::PopItemWidth();

    ImGui::SeparatorText("Список соседей");
    bool neighborListEnabled = simulation.isNeighborListEnabled();
    if (ImGui::Checkbox("NeighborList", &neighborListEnabled))
        simulation.setNeighborListEnabled(neighborListEnabled);

    int cellSize = simulation.sim_box.grid.cellSize;
    if (ImGui::SliderInt("Cell size", &cellSize, 1, 32)) {
        simulation.sim_box.setSizeBox(simulation.sim_box.size, cellSize);
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

    const float exitButtonWidth = ImGui::GetContentRegionAvail().x;
    const float footerHeight = ImGui::GetFrameHeightWithSpacing();
    const float remaining = ImGui::GetContentRegionAvail().y - footerHeight;
    if (remaining > 0.0f) {
        ImGui::Dummy(ImVec2(0.0f, remaining));
    }
    if (ImGui::Button("Exit", ImVec2(exitButtonWidth, 0.0f))) {
        AppSignals::UI::ExitApplication.emit();
    }

    ImGui::End();
}
