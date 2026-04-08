#include "ioPanel.h"

#include <imgui-SFML.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "App/AppSignals.h"
#include "Engine/Simulation.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"
#include "GUI/interface/style/ComboStyle.h"
#include "GUI/interface/UiState.h"

namespace {
    struct AtomTypeOption {
        const char* label;
        AtomData::Type type;
    };

    struct ParsedSceneInfo {
        std::string title;
        std::string description;
        unsigned imageWidth = 0;
        unsigned imageHeight = 0;
        bool hasEmbeddedPreview = false;
        std::vector<std::uint8_t> imageBytes;
    };

    constexpr std::array<AtomTypeOption, 19> kAtomTypeOptions{{
        {"Zerium", AtomData::Type::Z}, {"H", AtomData::Type::H},   {"He", AtomData::Type::He}, {"Li", AtomData::Type::Li},
        {"Be", AtomData::Type::Be},    {"B", AtomData::Type::B},   {"C", AtomData::Type::C},   {"N", AtomData::Type::N},
        {"O", AtomData::Type::O},      {"F", AtomData::Type::F},   {"Ne", AtomData::Type::Ne}, {"Na", AtomData::Type::Na},
        {"Mg", AtomData::Type::Mg},    {"Al", AtomData::Type::Al}, {"Si", AtomData::Type::Si}, {"P", AtomData::Type::P},
        {"S", AtomData::Type::S},      {"Cl", AtomData::Type::Cl}, {"Ar", AtomData::Type::Ar},
    }};

    std::string trim(std::string_view value) {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return std::string(value.substr(begin, end - begin));
    }

    std::string valueAfterTag(std::string_view line, std::string_view tag) {
        if (!line.starts_with(tag)) {
            return {};
        }
        return trim(line.substr(tag.size()));
    }

    std::vector<std::uint8_t> decodeBase64(std::string_view encoded) {
        std::array<int, 256> decodeTable{};
        decodeTable.fill(-1);

        constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i) {
            decodeTable[static_cast<unsigned char>(alphabet[i])] = i;
        }

        std::vector<std::uint8_t> decoded;
        decoded.reserve((encoded.size() / 4) * 3);

        int val = 0;
        int valb = -8;
        for (unsigned char c : encoded) {
            if (std::isspace(c)) {
                continue;
            }
            if (c == '=') {
                break;
            }
            const int decodedChar = decodeTable[c];
            if (decodedChar < 0) {
                continue;
            }
            val = (val << 6) + decodedChar;
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(static_cast<std::uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return decoded;
    }

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

    void drawCaptureStatus(const UiState& uiState) {
        const double blinkTime = uiState.captureBlinkElapsed;
        const int blinkStep = static_cast<int>(blinkTime);
        const bool blinkOn = (blinkStep % 2) == 0;
        const float alpha = uiState.captureRecording ? (blinkOn ? 0.8f : 0.2f) : 0.18f;
        const ImGuiStyle& style = ImGui::GetStyle();
        const float lineHeight = ImGui::GetFrameHeight();
        const float radius = std::max(4.0f, lineHeight * 0.24f);
        const float dotWidth = radius * 2.0f + style.ItemInnerSpacing.x * 0.35f;

        ImGui::SameLine();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const ImVec2 center(cursor.x + radius, cursor.y + lineHeight * 0.5f);

        ImGui::Dummy(ImVec2(dotWidth, lineHeight));
        ImGui::GetWindowDrawList()->AddCircleFilled(center, radius, ImGui::GetColorU32(ImVec4(0.95f, 0.16f, 0.16f, alpha)));
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("fps: %.1f", uiState.captureFps);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("frame: %llu", static_cast<unsigned long long>(uiState.captureFrameCount));
    }

    constexpr float kSceneTileRounding = 10.0f;

    void drawScenePreviewFallback(const ImVec2& previewSize) {
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 max(min.x + previewSize.x, min.y + previewSize.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(0.24f, 0.28f, 0.33f, 1.0f)), kSceneTileRounding);
        drawList->AddRect(min, max, ImGui::GetColorU32(ImVec4(0.36f, 0.42f, 0.50f, 1.0f)), kSceneTileRounding, 0, 1.0f);

        const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
        drawList->AddLine(ImVec2(min.x + 12.0f, max.y - 12.0f), ImVec2(center.x - 8.0f, center.y + 6.0f),
                          ImGui::GetColorU32(ImVec4(0.55f, 0.62f, 0.70f, 0.7f)), 2.0f);
        drawList->AddLine(ImVec2(center.x - 8.0f, center.y + 6.0f), ImVec2(center.x + 6.0f, center.y - 8.0f),
                          ImGui::GetColorU32(ImVec4(0.55f, 0.62f, 0.70f, 0.7f)), 2.0f);
        drawList->AddLine(ImVec2(center.x + 6.0f, center.y - 8.0f), ImVec2(max.x - 12.0f, max.y - 20.0f),
                          ImGui::GetColorU32(ImVec4(0.55f, 0.62f, 0.70f, 0.7f)), 2.0f);
        drawList->AddCircleFilled(ImVec2(max.x - 22.0f, min.y + 20.0f), 7.0f,
                                  ImGui::GetColorU32(ImVec4(0.60f, 0.68f, 0.78f, 0.75f)));
        ImGui::Dummy(previewSize);
    }

    ParsedSceneInfo parseSceneInfo(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return {};
        }

        ParsedSceneInfo info;
        std::string section;
        std::string imageEncoding;
        std::string imageFormat;
        std::string imageDataBase64;
        bool readingImageData = false;

        std::string line;
        while (std::getline(file, line)) {
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed.starts_with('#')) {
                continue;
            }

            if (readingImageData) {
                if (trimmed == "data_end") {
                    readingImageData = false;
                }
                else {
                    imageDataBase64 += trimmed;
                }
                continue;
            }

            if (trimmed.front() == '[') {
                section = trimmed;
                continue;
            }

            if (section == "[meta]") {
                if (trimmed.starts_with("title ")) {
                    info.title = valueAfterTag(trimmed, "title");
                }
                else if (trimmed.starts_with("description ")) {
                    info.description = valueAfterTag(trimmed, "description");
                }
            }
            else if (section == "[image]") {
                if (trimmed.starts_with("encoding ")) {
                    imageEncoding = valueAfterTag(trimmed, "encoding");
                }
                else if (trimmed.starts_with("format ")) {
                    imageFormat = valueAfterTag(trimmed, "format");
                }
                else if (trimmed.starts_with("width ")) {
                    info.imageWidth = static_cast<unsigned>(std::max(0, std::stoi(valueAfterTag(trimmed, "width"))));
                }
                else if (trimmed.starts_with("height ")) {
                    info.imageHeight = static_cast<unsigned>(std::max(0, std::stoi(valueAfterTag(trimmed, "height"))));
                }
                else if (trimmed == "data_begin") {
                    readingImageData = true;
                }
            }
        }

        if (info.title.empty()) {
            info.title = path.stem().string();
        }

        if (imageEncoding == "base64" && imageFormat == "rgba8" && info.imageWidth > 0 && info.imageHeight > 0) {
            info.imageBytes = decodeBase64(imageDataBase64);
            const size_t expectedSize = static_cast<size_t>(info.imageWidth) * static_cast<size_t>(info.imageHeight) * 4;
            info.hasEmbeddedPreview = (info.imageBytes.size() == expectedSize);
            if (!info.hasEmbeddedPreview) {
                info.imageBytes.clear();
            }
        }

        return info;
    }
}

void IOPanel::ensureSceneCatalogLoaded() {
    if (sceneCatalogLoaded_) {
        return;
    }

    sceneCatalogLoaded_ = true;
    sceneTiles_.clear();

    const std::filesystem::path scenesDir = "demo/scenes";
    if (!std::filesystem::exists(scenesDir) || !std::filesystem::is_directory(scenesDir)) {
        return;
    }

    std::vector<std::filesystem::path> scenePaths;
    for (const auto& entry : std::filesystem::directory_iterator(scenesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".lat") {
            scenePaths.push_back(entry.path());
        }
    }

    std::sort(scenePaths.begin(), scenePaths.end());
    sceneTiles_.reserve(scenePaths.size());

    for (const auto& path : scenePaths) {
        ParsedSceneInfo parsed = parseSceneInfo(path);

        SceneTile tile;
        tile.path = path.string();
        tile.title = std::move(parsed.title);
        tile.description = std::move(parsed.description);

        if (parsed.hasEmbeddedPreview) {
            sf::Image image;
            image.resize({parsed.imageWidth, parsed.imageHeight}, parsed.imageBytes.data());
            tile.hasPreview = tile.previewTexture.loadFromImage(image);
        }

        sceneTiles_.push_back(std::move(tile));
    }
}

void IOPanel::draw(float scale, sf::Vector2u windowSize, Simulation& simulation, FileDialogManager& fileDialog, UiState& uiState) {
    const float target = visible_ ? 1.f : 0.f;
    const float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress_ += (target - animProgress_) * std::min(step, 1.f);

    if (animProgress_ < 0.01f) {
        return;
    }

    ensureSceneCatalogLoaded();
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

    if (uiState.captureAvailable) {
        const char* captureLabel = uiState.captureRecording ? "Стоп" : "Запись";
        if (ImGui::Button(captureLabel, ImVec2(saveButtonWidth * scale, 0.f))) {
            AppSignals::Capture::ToggleRecording.emit();
        }
        drawCaptureStatus(uiState);
    }

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
        boxSize_.x = std::max(boxSize_.x, 1.0f);
        boxSize_.y = std::max(boxSize_.y, 1.0f);
        boxSize_.z = std::max(boxSize_.z, 1.0f);
        AppSignals::UI::ResizeBox.emit(boxSize_);
    }

    ImGui::SeparatorText("Массивогенератор");
    ImGui::SliderInt("##atoms_per_axis", &sceneAxisCount_, 2, 200);
    ImGui::SameLine();
    drawAtomTypeCombo("##atom_type", atomType_, 80.f * scale, scale);

    if (ImGui::Button("Создать##crystal", ImVec2(buttonWidth * scale, 0.f))) {
        AppSignals::UI::CreateCrystal.emit(sceneAxisCount_, atomType_, sceneIs3D_);
    }
    ImGui::SameLine();
    ImGui::Checkbox("3D", &sceneIs3D_);

    ImGui::SeparatorText("Газогенератор");
    ImGui::SliderInt("##gas_atom_count", &gasAtomCount_, 100, 300000);
    ImGui::SameLine();
    drawAtomTypeCombo("##atom_type_gas", gasAtomType_, 80.f * scale, scale);
    if (ImGui::Button("Создать##gas", ImVec2(buttonWidth * scale, 0.f))) {
        AppSignals::UI::CreateGas.emit(gasAtomCount_, gasAtomType_, gasIs3D_, gasDensity_);
    }
    ImGui::SameLine();
    ImGui::Checkbox("3D##gas", &gasIs3D_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f * scale);
    ImGui::SliderFloat("##gas_density", &gasDensity_, 0.25f, 3.0f, "%.2f");

    ImGui::SeparatorText("Сцены");
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float tileSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float tileWidth = std::max(80.0f, (availableWidth - tileSpacing) * 0.5f);
    const float tileHeight = tileWidth * 0.78f;
    const ImVec2 previewSize(tileWidth, tileHeight);

    for (size_t i = 0; i < sceneTiles_.size(); ++i) {
        SceneTile& tile = sceneTiles_[i];
        ImGui::PushID(static_cast<int>(i));

        if ((i % 2) != 0) {
            ImGui::SameLine();
        }

        ImGui::InvisibleButton("scene_tile", ImVec2(tileWidth, tileHeight));
        const ImVec2 tileMin = ImGui::GetItemRectMin();
        const ImVec2 tileMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(tileMin, tileMax, ImGui::GetColorU32(ImVec4(0.14f, 0.17f, 0.21f, 1.0f)), kSceneTileRounding);

        const bool isHovered = ImGui::IsItemHovered();
        if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            AppSignals::UI::LoadSimulation.emit(tile.path);
        }

        if (tile.hasPreview) {
            const ImTextureID textureId = static_cast<ImTextureID>(tile.previewTexture.getNativeHandle());
            const sf::Vector2u textureSize = tile.previewTexture.getSize();
            ImVec2 uvMin(0.0f, 0.0f);
            ImVec2 uvMax(1.0f, 1.0f);

            if (textureSize.x > 0 && textureSize.y > 0) {
                const float textureAspect = static_cast<float>(textureSize.x) / static_cast<float>(textureSize.y);
                const float tileAspect = tileWidth / tileHeight;

                if (textureAspect > tileAspect) {
                    const float visibleWidth = tileAspect / textureAspect;
                    const float crop = (1.0f - visibleWidth) * 0.5f;
                    uvMin.x = crop;
                    uvMax.x = 1.0f - crop;
                }
                else if (textureAspect < tileAspect) {
                    const float visibleHeight = textureAspect / tileAspect;
                    const float crop = (1.0f - visibleHeight) * 0.5f;
                    uvMin.y = crop;
                    uvMax.y = 1.0f - crop;
                }
            }

            drawList->AddImageRounded(textureId, tileMin, tileMax, uvMin, uvMax, IM_COL32_WHITE, kSceneTileRounding,
                                      ImDrawFlags_RoundCornersAll);
        }
        else {
            ImGui::SetCursorScreenPos(tileMin);
            drawScenePreviewFallback(previewSize);
        }

        const ImVec4 borderColor = isHovered ? ImVec4(0.38f, 0.64f, 1.00f, 1.0f) : ImVec4(0.30f, 0.36f, 0.42f, 1.0f);
        drawList->AddRect(tileMin, tileMax, ImGui::GetColorU32(borderColor), kSceneTileRounding, 0, isHovered ? 1.5f : 1.0f);

        const ImVec2 titlePos(tileMin.x + 10.0f, tileMax.y - 25.0f);
        drawList->AddText(ImVec2(titlePos.x + 1.0f, titlePos.y + 1.0f),
                          ImGui::GetColorU32(ImVec4(0.02f, 0.03f, 0.05f, 0.85f)), tile.title.c_str());
        drawList->AddText(titlePos, ImGui::GetColorU32(ImVec4(0.95f, 0.96f, 0.98f, 1.0f)), tile.title.c_str());
        if (!tile.description.empty()) {
            const ImVec2 descriptionPos(tileMin.x + 10.0f, tileMax.y - 15.0f);
            drawList->AddText(ImVec2(descriptionPos.x + 1.0f, descriptionPos.y + 1.0f),
                              ImGui::GetColorU32(ImVec4(0.02f, 0.03f, 0.05f, 0.80f)), tile.description.c_str());
            drawList->AddText(descriptionPos, ImGui::GetColorU32(ImVec4(0.82f, 0.86f, 0.90f, 0.98f)), tile.description.c_str());
        }

        ImGui::PopID();
    }

    ImGui::End();
}
