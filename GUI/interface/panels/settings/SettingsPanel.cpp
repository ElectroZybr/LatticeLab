#include "SettingsPanel.h"

#include <array>
#include <cstdio>

#include <imgui.h>

#include "App/AppSignals.h"
#include "App/UserSettings.h"
#include "App/capture/CaptureController.h"
#include "App/language_strings/GlobalStrings.h"
#include "Engine/Simulation.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"
#include "GUI/interface/style/ComboStyle.h"
#include "Rendering/BaseRenderer.h"
#include "generated/AppVersion.h"

namespace {
    const char* integratorName(Integrator::Scheme scheme) {
        switch (scheme) {
        case Integrator::Scheme::Verlet:
            return global_strings->integrator_velocity_verlet.c_str();
        case Integrator::Scheme::KDK:
            return global_strings->integrator_kdk.c_str();
        case Integrator::Scheme::RK4:
            return global_strings->integrator_runge_kutta_4.c_str();
        case Integrator::Scheme::Langevin:
            return global_strings->integrator_langevin.c_str();
        }

        return global_strings->integrator_unknown.c_str();
    }

    const char* speedColorModeName(IRenderer::SpeedColorMode mode) {
        switch (mode) {
        case IRenderer::SpeedColorMode::AtomColor:
            return global_strings->speed_color_normal_coloring.c_str();
        case IRenderer::SpeedColorMode::GradientClassic:
            return global_strings->speed_color_gradient_coloring.c_str();
        case IRenderer::SpeedColorMode::GradientTurbo:
            return global_strings->speed_color_turbo_coloring.c_str();
        }
        return global_strings->speed_color_normal_coloring.c_str();
    }

    const char* capturePresetName(CaptureSettings::Preset preset) {
        switch (preset) {
        case CaptureSettings::Preset::Ultrafast:
            return global_strings->capture_preset_ultrafast.c_str();
        case CaptureSettings::Preset::Veryfast:
            return global_strings->capture_preset_veryfast.c_str();
        case CaptureSettings::Preset::Faster:
            return global_strings->capture_preset_faster.c_str();
        case CaptureSettings::Preset::Fast:
            return global_strings->capture_preset_fast.c_str();
        case CaptureSettings::Preset::Medium:
            return global_strings->capture_preset_medium.c_str();
        }
        return global_strings->capture_preset_veryfast.c_str();
    }

    const char* capturePixelFormatName(CaptureSettings::PixelFormat pixelFormat) {
        switch (pixelFormat) {
        case CaptureSettings::PixelFormat::Yuv420p:
            return global_strings->capture_pixel_format_Yuv420p.c_str();
        case CaptureSettings::PixelFormat::Yuv444p:
            return global_strings->capture_pixel_format_Yuv444p.c_str();
        }
        return global_strings->capture_pixel_format_Yuv444p.c_str();
    }
}

void SettingsPanel::draw(float uiScale, Vec2i windowSize, Simulation& simulation, std::unique_ptr<IRenderer>& renderer,
                         CaptureController& captureController, FileDialogManager& fileDialog) {
    float target = visible ? 1.f : 0.f;
    float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress += (target - animProgress) * std::min(step, 1.f);

    if (animProgress < 0.01f) {
        return;
    }

    const float panelWidth = 300.f * uiScale;
    const float topOffset = 65.f * uiScale;
    const float panelHeight = static_cast<float>(windowSize.y) - topOffset;

    const float x = -panelWidth + panelWidth * animProgress;

    ImGui::SetNextWindowPos(ImVec2(x, topOffset));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin(global_strings->imgui_settings_panel.c_str(), nullptr, PANEL_FLAGS);

    ImGui::SeparatorText(global_strings->imgui_simulation.c_str());

    ImGui::TextUnformatted(global_strings->imgui_gravity.c_str());
    ImGui::SameLine();
    Vec3f gravity = simulation.getGravity();
    if (ImGui::Button(global_strings->imgui_reset_gravity.c_str(), ImVec2(50.f * uiScale, 0.f))) {
        simulation.setGravity(Vec3f(0, 0, 0));
        gravity = simulation.getGravity();
    }
    float gx = gravity.x;
    float gy = gravity.y;
    float gz = gravity.z;
    bool gravityChanged = false;
    gravityChanged |= ImGui::SliderFloat(global_strings->imgui_gravity_x.c_str(), &gx, -10.0f, 10.0f, "%.2f");
    gravityChanged |= ImGui::SliderFloat(global_strings->imgui_gravity_y.c_str(), &gy, -10.0f, 10.0f, "%.2f");
    gravityChanged |= ImGui::SliderFloat(global_strings->imgui_gravity_z.c_str(), &gz, -10.0f, 10.0f, "%.2f");
    if (gravityChanged) {
        simulation.setGravity(Vec3f(gx, gy, gz));
    }

    Integrator::Scheme currentIntegrator = simulation.getIntegrator();
    if (ComboStyle::beginCombo(global_strings->imgui_integrator.c_str(), integratorName(currentIntegrator), 0.0f, uiScale)) {
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
        ImGui::TextWrapped(global_strings->imgui_warning_not_implemented_used_as_velocity_verlet.c_str(), integratorName(currentIntegrator));
        ImGui::PopStyleColor();
    }

    float maxParticleSpeed = simulation.getMaxParticleSpeed();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat(global_strings->imgui_speed_of_light.c_str(), &maxParticleSpeed, 0.0f, 100.0f,
                           maxParticleSpeed <= 0.0f ? global_strings->imgui_speed_of_light_unlimited.c_str() : "%.2f")) {
        simulation.setMaxParticleSpeed(maxParticleSpeed);
    }
    ImGui::PopItemWidth();

    float accelDamping = simulation.getAccelDamping();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat(global_strings->imgui_accel_damping.c_str(), &accelDamping, 0.0f, 1.0f, "%.3f")) {
        simulation.setAccelDamping(accelDamping);
    }
    ImGui::PopItemWidth();

    float dt = simulation.getDt();
    ImGui::PushItemWidth(150.0f * uiScale);
    if (ImGui::SliderFloat(global_strings->imgui_time_step.c_str(), &dt, 0.0001f, 0.05f, "%.4f",
                           ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic)) {
        simulation.setDt(dt);
    }
    ImGui::PopItemWidth();

    bool bondFormationEnabled = simulation.isBondFormationEnabled();
    if (ImGui::Checkbox(global_strings->imgui_bond_formation.c_str(), &bondFormationEnabled)) {
        simulation.setBondFormationEnabled(bondFormationEnabled);
    }

    bool ljEnabled = simulation.isLJEnabled();
    if (ImGui::Checkbox(global_strings->imgui_lj.c_str(), &ljEnabled)) {
        simulation.setLJEnabled(ljEnabled);
    }
    ImGui::SameLine();
    bool coulombEnabled = simulation.isCoulombEnabled();
    if (ImGui::Checkbox(global_strings->imgui_coulomb.c_str(), &coulombEnabled)) {
        simulation.setCoulombEnabled(coulombEnabled);
    }

    ImGui::SeparatorText(global_strings->imgui_render.c_str());
    ImGui::Checkbox(global_strings->imgui_grid.c_str(), &renderer->drawGrid);
    ImGui::Checkbox(global_strings->imgui_connections.c_str(), &renderer->drawBonds);

    ImGui::TextUnformatted(global_strings->imgui_color_scheme.c_str());
    IRenderer::SpeedColorMode speedMode = renderer->speedColorMode;
    if (ComboStyle::beginCombo(global_strings->imgui_speed_color_mode.c_str(), speedColorModeName(speedMode), 220.0f * uiScale, uiScale,
                               ImGuiComboFlags_HeightLargest)) {
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

    ImGui::TextUnformatted(global_strings->imgui_max_gradien_velocity.c_str());

    static float manualSpeedGradientMax = 5.0f;
    bool autoSpeedGradient = renderer->speedGradientMax <= 0.0f;
    const bool gradientModeEnabled = renderer->speedColorMode != IRenderer::SpeedColorMode::AtomColor;
    if (!autoSpeedGradient) {
        manualSpeedGradientMax = renderer->speedGradientMax;
    }

    ImGui::PushItemWidth(180.0f * uiScale);
    ImGui::BeginDisabled(autoSpeedGradient || !gradientModeEnabled);
    if (ImGui::SliderFloat(global_strings->imgui_speed_gradient_max_slider.c_str(), &manualSpeedGradientMax, 0.1f, 10.0f, "%.2f")) {
        renderer->speedGradientMax = manualSpeedGradientMax;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!gradientModeEnabled);
    if (ImGui::Checkbox(global_strings->imgui_auto_speed_gradien.c_str(), &autoSpeedGradient)) {
        renderer->speedGradientMax = autoSpeedGradient ? 0.0f : manualSpeedGradientMax;
    }
    ImGui::EndDisabled();
    ImGui::PopItemWidth();

    ImGui::SeparatorText(global_strings->imgui_neighbour_list.c_str());
    int cellSize = simulation.box().grid.cellSize;
    if (ImGui::SliderInt(global_strings->imgui_cell_size.c_str(), &cellSize, 1, 32)) {
        simulation.setSizeBox(simulation.box().size, cellSize);
    }

    float cutoff = simulation.getNeighborListCutoff();
    if (ImGui::SliderFloat(global_strings->imgui_cutoff_nl.c_str(), &cutoff, 0.5f, 20.0f, "%.2f")) {
        simulation.setNeighborListCutoff(cutoff);
    }

    float skin = simulation.getNeighborListSkin();
    if (ImGui::SliderFloat(global_strings->imgui_skin_nl.c_str(), &skin, 0.1f, 10.0f, "%.2f")) {
        simulation.setNeighborListSkin(skin);
    }

    if (captureController.isAvailable()) {
        ImGui::SeparatorText(global_strings->imgui_write.c_str());
        CaptureSettings captureSettings = captureController.settings();
        const bool recordingActive = captureController.isRecording();
        const std::string captureDir = captureController.outputDirectory().string();

        ImGui::TextUnformatted(global_strings->imgui_video_saving_folder.c_str());
        std::array<char, 512> captureDirBuffer{};
        std::snprintf(captureDirBuffer.data(), captureDirBuffer.size(), "%s", captureDir.data());
        const float browseButtonWidth = ImGui::GetFrameHeight();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseButtonWidth - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText(global_strings->imgui_capture_dir.c_str(), captureDirBuffer.data(), captureDirBuffer.size(),
                         ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button(global_strings->imgui_capture_dir_browse.c_str(), ImVec2(browseButtonWidth, 0.0f))) {
            fileDialog.openCaptureDirectory(captureDir);
        }

        ImGui::BeginDisabled(recordingActive);

        if (ImGui::BeginTable(global_strings->imgui_capture_settings_table.c_str(), 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int captureFps = captureSettings.fps;
            if (ImGui::SliderInt(global_strings->imgui_fps_capture.c_str(), &captureFps, 10, 60)) {
                captureSettings.fps = captureFps;
                captureController.setSettings(captureSettings);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int crf = captureSettings.crf;
            if (ImGui::SliderInt(global_strings->imgui_crf_capture.c_str(), &crf, 12, 30)) {
                captureSettings.crf = crf;
                captureController.setSettings(captureSettings);
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            CaptureSettings::Preset preset = captureSettings.preset;
            if (ComboStyle::beginCombo(global_strings->imgui_preset_capture.c_str(), capturePresetName(preset), -FLT_MIN, uiScale)) {
                const CaptureSettings::Preset presets[] = {
                    CaptureSettings::Preset::Ultrafast, CaptureSettings::Preset::Veryfast, CaptureSettings::Preset::Faster,
                    CaptureSettings::Preset::Fast,      CaptureSettings::Preset::Medium,
                };

                for (CaptureSettings::Preset candidate : presets) {
                    const bool isSelected = (candidate == preset);
                    if (ImGui::Selectable(capturePresetName(candidate), isSelected)) {
                        captureSettings.preset = candidate;
                        captureController.setSettings(captureSettings);
                        preset = candidate;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TableSetColumnIndex(1);
            CaptureSettings::PixelFormat pixelFormat = captureSettings.pixelFormat;
            if (ComboStyle::beginCombo(global_strings->imgui_color_capture.c_str(), capturePixelFormatName(pixelFormat), -FLT_MIN,
                                       uiScale)) {
                const CaptureSettings::PixelFormat pixelFormats[] = {
                    CaptureSettings::PixelFormat::Yuv444p,
                    CaptureSettings::PixelFormat::Yuv420p,
                };

                for (CaptureSettings::PixelFormat candidate : pixelFormats) {
                    const bool isSelected = (candidate == pixelFormat);
                    if (ImGui::Selectable(capturePixelFormatName(candidate), isSelected)) {
                        captureSettings.pixelFormat = candidate;
                        captureController.setSettings(captureSettings);
                        pixelFormat = candidate;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::EndTable();
        }

        ImGui::EndDisabled();
    }

    if (ImGui::Button(global_strings->imgui_reset_settings.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
        const UserSettings defaults;
        captureController.setOutputDirectory(defaults.captureOutputDirectory);
        captureController.setSettings(defaults.captureSettings);

        renderer->drawGrid = defaults.rendererDrawGrid;
        renderer->drawBonds = defaults.rendererDrawBonds;
        renderer->speedColorMode = defaults.rendererSpeedColorMode;
        renderer->speedGradientMax = defaults.rendererSpeedGradientMax;

        simulation.setIntegrator(defaults.simulationIntegrator);
        simulation.setBondFormationEnabled(defaults.simulationBondFormationEnabled);
        simulation.setLJEnabled(defaults.simulationLJEnabled);
        simulation.setCoulombEnabled(defaults.simulationCoulombEnabled);
    }

    const float exitButtonWidth = ImGui::GetContentRegionAvail().x;
    const std::string versionText = global_strings->version_text_pre.str() + LATTICELAB_VERSION_STRING + global_strings->version_text_after.str();
    const float versionWidth = ImGui::CalcTextSize(versionText.c_str()).x;
    const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing();
    const float remaining = ImGui::GetContentRegionAvail().y - footerHeight;
    if (remaining > 0.0f) {
        ImGui::Dummy(ImVec2(0.0f, remaining));
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.76f, 0.80f, 0.85f));
    ImGui::SetCursorPosX(std::max(0.0f, (ImGui::GetContentRegionAvail().x - versionWidth) * 0.5f));
    ImGui::TextUnformatted(versionText.c_str());
    ImGui::PopStyleColor();

    if (ImGui::Button(global_strings->imgui_exit_button.c_str(), ImVec2(exitButtonWidth, 0.0f))) {
        AppSignals::UI::ExitApplication.emit();
    }

    ImGui::End();
}
