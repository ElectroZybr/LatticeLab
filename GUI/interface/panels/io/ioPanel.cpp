#include "ioPanel.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "App/AppSignals.h"
#include "Engine/Simulation.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"
#include "GUI/interface/interface.h"
#include "GUI/interface/style/ComboStyle.h"

namespace {
struct AtomTypeOption {
    const char* label;
    AtomData::Type type;
};

constexpr std::array<AtomTypeOption, 19> kAtomTypeOptions{{
    {"Zerium", AtomData::Type::Z},
    {"H", AtomData::Type::H},
    {"He", AtomData::Type::He},
    {"Li", AtomData::Type::Li},
    {"Be", AtomData::Type::Be},
    {"B", AtomData::Type::B},
    {"C", AtomData::Type::C},
    {"N", AtomData::Type::N},
    {"O", AtomData::Type::O},
    {"F", AtomData::Type::F},
    {"Ne", AtomData::Type::Ne},
    {"Na", AtomData::Type::Na},
    {"Mg", AtomData::Type::Mg},
    {"Al", AtomData::Type::Al},
    {"Si", AtomData::Type::Si},
    {"P", AtomData::Type::P},
    {"S", AtomData::Type::S},
    {"Cl", AtomData::Type::Cl},
    {"Ar", AtomData::Type::Ar},
}};

int findTypeIndex(AtomData::Type type) {
    for (int i = 0; i < static_cast<int>(kAtomTypeOptions.size()); ++i) {
        if (kAtomTypeOptions[static_cast<size_t>(i)].type == type) {
            return i;
        }
    }
    return 0;
}

void drawAtomTypeCombo(const char* id, AtomData::Type& atomType, float width, float uiScale) {
    int selectedTypeIndex = findTypeIndex(atomType);
    const char* selectedLabel = kAtomTypeOptions[static_cast<size_t>(selectedTypeIndex)].label;

    if (ComboStyle::beginCenteredCombo(id, width, uiScale)) {
        ComboStyle::pushCenteredSelectableText();
        for (int i = 0; i < static_cast<int>(kAtomTypeOptions.size()); ++i) {
            const bool selected = (i == selectedTypeIndex);
            if (ImGui::Selectable(kAtomTypeOptions[static_cast<size_t>(i)].label, selected)) {
                atomType = kAtomTypeOptions[static_cast<size_t>(i)].type;
                selectedTypeIndex = i;
                selectedLabel = kAtomTypeOptions[static_cast<size_t>(i)].label;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ComboStyle::popCenteredSelectableText();
        ImGui::EndCombo();
    }

    ComboStyle::drawCenteredComboPreview(selectedLabel);
}

void drawCaptureStatus() {
    const float blinkPhase = std::fmod(static_cast<float>(ImGui::GetTime()), 1.0f);
    const float alpha = Interface::captureRecording
        ? (blinkPhase < 0.5f ? 1.0f : 0.2f)
        : 0.18f;
    const float textHeight = ImGui::GetTextLineHeight();
    const float radius = 5.f;
    const float dotWidth = radius * 2.0f;

    ImGui::SameLine();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 center(cursor.x + radius, cursor.y + textHeight * 0.5f + 3.0f);

    ImGui::Dummy(ImVec2(dotWidth, textHeight));
    ImGui::GetWindowDrawList()->AddCircleFilled(
        center,
        radius,
        ImGui::GetColorU32(ImVec4(0.95f, 0.16f, 0.16f, alpha))
    );
    ImGui::SameLine();
    ImGui::Text("fps: %.1f", Interface::captureFps);
    ImGui::SameLine();
    ImGui::Text("frame: %llu", static_cast<unsigned long long>(Interface::captureFrameCount));
}
} // namespace

void IOPanel::draw(float scale, sf::Vector2u windowSize, Simulation& simulation, FileDialogManager& fileDialog) {
    const float target = visible_ ? 1.f : 0.f;
    const float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress_ += (target - animProgress_) * std::min(step, 1.f);

    if (animProgress_ < 0.01f) return;

    boxSize_ = simulation.box().size;

    const float panelWidth = 300.f * scale;
    const float topOffset = 65.f * scale;
    const float panelHeight = static_cast<float>(windowSize.y) - topOffset;
    const float x = -panelWidth + panelWidth * animProgress_;
    const float buttonWidth = 140.f;
    const float saveButtonWidth = 84.f;

    ImGui::SetNextWindowPos(ImVec2(x, topOffset));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin("##IOPanel", nullptr, PANEL_FLAGS);

    ImGui::SeparatorText("Файл");
    if (ImGui::Button("Загрузить", ImVec2(saveButtonWidth * scale, 0.f))) {
        fileDialog.openLoad();
    }
    ImGui::SameLine();
    if (ImGui::Button("Сохранить", ImVec2(saveButtonWidth * scale, 0.f))) {
        fileDialog.openSave();
    }
    ImGui::SameLine();
    if (ImGui::Button("Очистить", ImVec2(saveButtonWidth * scale, 0.f))) {
        AppSignals::UI::ClearSimulation.emit();
    }

    const char* captureLabel = Interface::captureRecording ? "Стоп" : "Запись";
    if (ImGui::Button(captureLabel, ImVec2(saveButtonWidth * scale, 0.f))) {
        AppSignals::UI::ToggleCapture.emit();
    }
    drawCaptureStatus();

    ImGui::SeparatorText("Размер бокса");
    bool boxSizeChanged = false;
    boxSizeChanged |= ImGui::SliderFloat("X##box_size_x", &boxSize_.x, 5.0f, 400.0f, "%.1f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f * scale);
    boxSizeChanged |= ImGui::InputFloat("##box_size_x_input", &boxSize_.x, 0.0f, 0.0f, "%.1f");

    boxSizeChanged |= ImGui::SliderFloat("Y##box_size_y", &boxSize_.y, 5.0f, 400.0f, "%.1f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f * scale);
    boxSizeChanged |= ImGui::InputFloat("##box_size_y_input", &boxSize_.y, 0.0f, 0.0f, "%.1f");

    boxSizeChanged |= ImGui::SliderFloat("Z##box_size_z", &boxSize_.z, 5.0f, 200.0f, "%.1f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f * scale);
    boxSizeChanged |= ImGui::InputFloat("##box_size_z_input", &boxSize_.z, 0.0f, 0.0f, "%.1f");

    if (boxSizeChanged) {
        if (boxSize_.x < 1.f) boxSize_.x = 1.f;
        if (boxSize_.y < 1.f) boxSize_.y = 1.f;
        if (boxSize_.z < 1.f) boxSize_.z = 1.f;
        AppSignals::UI::ResizeBox.emit(boxSize_);
    }

    ImGui::SeparatorText("Массивогенератор");
    ImGui::SliderInt("##Атомов по оси", &sceneAxisCount_, 2, 200);
    ImGui::SameLine();
    drawAtomTypeCombo("##atom_type", atomType_, 80.f * scale, scale);

    if (ImGui::Button("Создать##crystal", ImVec2(buttonWidth * scale, 0.f))) {
        AppSignals::UI::CreateCrystal.emit();
    }
    ImGui::SameLine();
    ImGui::Checkbox("3D", &sceneIs3D_);

    ImGui::SeparatorText("Газогенератор");
    ImGui::SliderInt("##gas_atom_count", &gasAtomCount_, 100, 300000);
    ImGui::SameLine();
    drawAtomTypeCombo("##atom_type_gas", gasAtomType_, 80.f * scale, scale);
    if (ImGui::Button("Создать##gas", ImVec2(buttonWidth * scale, 0.f))) {
        AppSignals::UI::CreateGas.emit();
    }
    ImGui::SameLine();
    ImGui::Checkbox("3D##gas", &gasIs3D_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f * scale);
    ImGui::SliderFloat("##gas_density", &gasDensity_, 0.25f, 3.0f, "%.2f");

    ImGui::End();
}
