#include "ioPanel.h"

#include <algorithm>

#include "GUI/interface/file_dialog/FileDialogManager.h"

void IOPanel::draw(float scale, sf::Vector2u windowSize, FileDialogManager& fileDialog) {
    const float target = visible_ ? 1.f : 0.f;
    const float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress_ += (target - animProgress_) * std::min(step, 1.f);

    if (animProgress_ < 0.01f) return;

    const float panelWidth = 300.f * scale;
    const float topOffset = 65.f * scale;
    const float panelHeight = static_cast<float>(windowSize.y) - topOffset;
    const float x = -panelWidth + panelWidth * animProgress_;
    const float buttonWidth = 140.f;

    ImGui::SetNextWindowPos(ImVec2(x, topOffset));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin("##IOPanel", nullptr, PANEL_FLAGS);

    ImGui::SeparatorText("Файл");
    if (ImGui::Button("Загрузить", ImVec2(buttonWidth * scale, 0.f))) {
        fileDialog.openLoad();
    }
    ImGui::SameLine();
    if (ImGui::Button("Сохранить", ImVec2(buttonWidth * scale, 0.f))) {
        fileDialog.openSave();
    }

    ImGui::SeparatorText("Генератор");
    ImGui::SliderInt("##Атомов по оси", &sceneAxisCount_, 2, 200);
    ImGui::SameLine();
    ImGui::Checkbox("3D", &sceneIs3D_);

    if (ImGui::Button("Создать", ImVec2(buttonWidth * scale, 0.f))) {
        pendingResult_ = IOCommand::CreateCrystal;
    }
    ImGui::SameLine();
    if (ImGui::Button("Очистить", ImVec2(buttonWidth * scale, 0.f))) {
        pendingResult_ = IOCommand::ClearSimulation;
    }

    ImGui::End();
}

std::optional<IOCommand> IOPanel::popResult() {
    auto result = pendingResult_;
    pendingResult_ = std::nullopt;
    return result;
}
