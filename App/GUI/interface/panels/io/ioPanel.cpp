#include "ioPanel.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

#include "App/AppSignals.h"
#include "Lattice/Engine/Simulation.h"
#include "GUI/interface/UiState.h"
#include "GUI/interface/file_dialog/FileDialogManager.h"
#include "GUI/interface/panels/io/ioPanelWidgets.h"

namespace {
    constexpr float kSceneTileRounding = 10.0f;
    constexpr int kRecordingFormatVideo = static_cast<int>(IOPanel::RecordingFormat::MP4);
    constexpr int kRecordingFormatXYZ = static_cast<int>(IOPanel::RecordingFormat::XYZ);

    const char* generatorLabel(IOPanel::GeneratorKind kind) {
        switch (kind) {
        case IOPanel::GeneratorKind::TriangularBipyramid:
            return "Triangular bipyramid";
        case IOPanel::GeneratorKind::RandomFill:
            return "Random fill";
        case IOPanel::GeneratorKind::LatticeFill:
            return "Lattice fill";
        }
        return "Triangular bipyramid";
    }

    const char* regionLabel(AppSignals::UI::GeneratorRegionKind kind) {
        switch (kind) {
        case AppSignals::UI::GeneratorRegionKind::Box:
            return "Box";
        case AppSignals::UI::GeneratorRegionKind::Sphere:
            return "Sphere";
        case AppSignals::UI::GeneratorRegionKind::Cylinder:
            return "Cylinder";
        case AppSignals::UI::GeneratorRegionKind::Capsule:
            return "Capsule";
        case AppSignals::UI::GeneratorRegionKind::Torus:
            return "Torus";
        case AppSignals::UI::GeneratorRegionKind::TrianglePyramid:
            return "Triangle pyramid";
        case AppSignals::UI::GeneratorRegionKind::TriangleBiPyramid:
            return "Triangle bipyramid";
        }
        return "Box";
    }

    const char* latticeStructureLabel(Generators::LatticeStructure structure) {
        switch (structure) {
        case Generators::LatticeStructure::Bcc:
            return "BCC";
        case Generators::LatticeStructure::Hex:
            return "HEX";
        }
        return "BCC";
    }

    const char* spawnModeLabel(Generators::SpawnMode mode) {
        switch (mode) {
        case Generators::SpawnMode::Reset:
            return "Reset";
        case Generators::SpawnMode::Add:
            return "Add";
        case Generators::SpawnMode::Replace:
            return "Replace";
        }
        return "Replace";
    }

    void drawAxisCountControls(const char* prefix, glm::ivec3& counts, float scale, int minValue, int maxValue, bool enableZ) {
        ImGui::SetNextItemWidth(160.0f * scale);
        ImGui::SliderInt((std::string("X##") + prefix).c_str(), &counts.x, minValue, maxValue);
        ImGui::SetNextItemWidth(160.0f * scale);
        ImGui::SliderInt((std::string("Y##") + prefix).c_str(), &counts.y, minValue, maxValue);
        ImGui::BeginDisabled(!enableZ);
        if (!enableZ) {
            counts.z = 1;
        }
        ImGui::SetNextItemWidth(160.0f * scale);
        ImGui::SliderInt((std::string("Z##") + prefix).c_str(), &counts.z, minValue, maxValue);
        ImGui::EndDisabled();
    }

    glm::ivec3 makeUniformAxisCounts(int count, bool enableZ) {
        return glm::ivec3(count, count, enableZ ? count : 1);
    }

    void rebalanceCompositionFractions(std::vector<AppSignals::UI::GeneratorComposeSpec>& composition, size_t lockedIndex) {
        if (composition.empty()) {
            return;
        }

        if (lockedIndex >= composition.size()) {
            lockedIndex = composition.size() - 1;
        }

        composition[lockedIndex].fraction = std::clamp(composition[lockedIndex].fraction, 0.0f, 1.0f);
        const float lockedFraction = composition[lockedIndex].fraction;
        const float remainingTotal = std::max(0.0f, 1.0f - lockedFraction);

        std::vector<size_t> otherIndices;
        otherIndices.reserve(composition.size() > 0 ? composition.size() - 1 : 0);
        float otherWeightSum = 0.0f;
        for (size_t i = 0; i < composition.size(); ++i) {
            if (i == lockedIndex) {
                continue;
            }
            composition[i].fraction = std::max(composition[i].fraction, 0.0f);
            otherIndices.push_back(i);
            otherWeightSum += composition[i].fraction;
        }

        if (otherIndices.empty()) {
            return;
        }

        if (otherWeightSum > 0.0f) {
            for (size_t i : otherIndices) {
                composition[i].fraction = remainingTotal * (composition[i].fraction / otherWeightSum);
            }
            return;
        }

        const float evenFraction = remainingTotal / static_cast<float>(otherIndices.size());
        for (size_t i : otherIndices) {
            composition[i].fraction = evenFraction;
        }
    }

    void drawRegionEditor(const char* prefix, AppSignals::UI::GeneratorRegionSpec& region, float scale) {
        if (ImGui::BeginCombo((std::string("Region##") + prefix).c_str(), regionLabel(region.kind))) {
            constexpr AppSignals::UI::GeneratorRegionKind regionKinds[] = {
                AppSignals::UI::GeneratorRegionKind::Box,
                AppSignals::UI::GeneratorRegionKind::Sphere,
                AppSignals::UI::GeneratorRegionKind::Cylinder,
                AppSignals::UI::GeneratorRegionKind::Capsule,
                AppSignals::UI::GeneratorRegionKind::Torus,
                AppSignals::UI::GeneratorRegionKind::TrianglePyramid,
                AppSignals::UI::GeneratorRegionKind::TriangleBiPyramid,
            };

            for (AppSignals::UI::GeneratorRegionKind kind : regionKinds) {
                const bool selected = kind == region.kind;
                if (ImGui::Selectable(regionLabel(kind), selected)) {
                    region.kind = kind;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(220.0f * scale);
        ImGui::DragFloat3((std::string("Center##") + prefix).c_str(), &region.center.x, 0.25f, -10000.0f, 10000.0f, "%.2f");

        switch (region.kind) {
        case AppSignals::UI::GeneratorRegionKind::Box:
            ImGui::SetNextItemWidth(220.0f * scale);
            ImGui::DragFloat3((std::string("Size##") + prefix).c_str(), &region.boxSize.x, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::Sphere:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Radius##") + prefix).c_str(), &region.sphereRadius, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::Cylinder:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Base radius##") + prefix).c_str(), &region.cylinderRadius, 0.25f, 0.0f, 10000.0f, "%.2f");
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Height##") + prefix).c_str(), &region.cylinderHeight, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::Capsule:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Capsule radius##") + prefix).c_str(), &region.capsuleRadius, 0.25f, 0.0f, 10000.0f, "%.2f");
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Capsule height##") + prefix).c_str(), &region.capsuleHeight, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::Torus:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Major radius##") + prefix).c_str(), &region.torusMajorRadius, 0.25f, 0.0f, 10000.0f, "%.2f");
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Tube radius##") + prefix).c_str(), &region.torusTubeRadius, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::TrianglePyramid:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Base circumradius##") + prefix).c_str(), &region.pyramidBaseCircumradius, 0.25f, 0.0f, 10000.0f,
                             "%.2f");
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Pyramid height##") + prefix).c_str(), &region.pyramidHeight, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        case AppSignals::UI::GeneratorRegionKind::TriangleBiPyramid:
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Base circumradius##") + prefix).c_str(), &region.bipyramidBaseCircumradius, 0.25f, 0.0f,
                             10000.0f, "%.2f");
            ImGui::SetNextItemWidth(120.0f * scale);
            ImGui::DragFloat((std::string("Bipyramid height##") + prefix).c_str(), &region.bipyramidHeight, 0.25f, 0.0f, 10000.0f, "%.2f");
            break;
        }
    }

    void drawCompositionEditor(const char* prefix, std::vector<AppSignals::UI::GeneratorComposeSpec>& composition, float scale, const char* defaultSpecies) {
        constexpr float kSpeciesWidth = 100.0f;
        constexpr float kFractionWidth = 72.0f;
        constexpr float kRemoveWidth = 22.0f;
        const float speciesWidth = std::floor(kSpeciesWidth * scale);
        const float fractionWidth = std::floor(kFractionWidth * scale);
        const float removeWidth = std::floor(kRemoveWidth * scale);
        const float rowWidth = speciesWidth + fractionWidth + removeWidth + 2.0f * ImGui::GetStyle().ItemSpacing.x;
        float totalFraction = 0.0f;
        for (size_t i = 0; i < composition.size(); ++i) {
            AppSignals::UI::GeneratorComposeSpec& entry = composition[i];
            totalFraction += std::max(entry.fraction, 0.0f);

            ImGui::PushID(static_cast<int>(i));
            std::array<char, 64> speciesBuffer{};
            std::snprintf(speciesBuffer.data(), speciesBuffer.size(), "%s", entry.species.c_str());
            ImGui::SetNextItemWidth(speciesWidth);
            if (ImGui::InputText((std::string("##species_") + prefix).c_str(), speciesBuffer.data(), speciesBuffer.size())) {
                entry.species = speciesBuffer.data();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(fractionWidth);
            float percent = std::clamp(entry.fraction, 0.0f, 1.0f) * 100.0f;
            const bool percentChanged =
                ImGui::DragFloat((std::string("##fraction_") + prefix).c_str(), &percent, 0.25f, 0.0f, 100.0f, "%.1f%%");
            if (percentChanged) {
                entry.fraction = std::clamp(percent / 100.0f, 0.0f, 1.0f);
                rebalanceCompositionFractions(composition, i);
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(composition.size() <= 1);
            const bool remove = ImGui::Button((std::string("x##") + prefix).c_str(), ImVec2(removeWidth, 0.0f));
            ImGui::EndDisabled();
            ImGui::PopID();

            if (remove) {
                composition.erase(composition.begin() + static_cast<std::ptrdiff_t>(i));
                --i;
            }
        }

        if (ImGui::Button((std::string("Add type##") + prefix).c_str(), ImVec2(140 * scale, 0.0f))) {
            composition.push_back({
                .species = defaultSpecies,
                .fraction = 0.0f,
            });
            rebalanceCompositionFractions(composition, composition.size() - 1);
        }

        totalFraction = 0.0f;
        for (const AppSignals::UI::GeneratorComposeSpec& entry : composition) {
            totalFraction += std::max(entry.fraction, 0.0f);
        }
        ImGui::SameLine();
        ImGui::Text("sum %.1f%%", totalFraction * 100.0f);
    }
}

void IOPanel::ensureSceneCatalogLoaded() {
    if (sceneCatalogLoaded_) {
        return;
    }

    try {
        sceneTiles_ = loadIOPanelSceneTiles(scenesDirectory_.string());
    }
    catch (const std::exception&) {
        sceneTiles_.clear();
    }
    sceneCatalogLoaded_ = true;
}

void IOPanel::clearPendingDeleteState() {
    pendingDeleteScenePath_.clear();
    pendingDeleteSceneTitle_.clear();
    pendingDeleteError_.clear();
}

void IOPanel::setScenesDirectory(std::filesystem::path scenesDirectory) {
    scenesDirectory_ = std::move(scenesDirectory);
    sceneCatalogLoaded_ = false;
}

void IOPanel::removeSceneTileByPath(std::string_view path) {
    const auto it =
        std::find_if(sceneTiles_.begin(), sceneTiles_.end(), [&](const IOPanelSceneTile& sceneTile) { return sceneTile.path == path; });
    if (it != sceneTiles_.end()) {
        sceneTiles_.erase(it);
    }
}

void IOPanel::draw(float scale, glm::ivec2 windowSize, Lattice::Simulation& simulation, FileDialogManager& fileDialog, UiState& uiState) {
    const float target = visible_ ? 1.f : 0.f;
    const float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress_ += (target - animProgress_) * std::min(step, 1.f);

    if (animProgress_ < 0.01f) {
        return;
    }

    if (fileDialog.hasSelectedSceneDirectory()) {
        setScenesDirectory(fileDialog.consumeSelectedSceneDirectory());
    }
    if (fileDialog.hasSavedSimulationPath()) {
        const std::filesystem::path savedSimulationPath = fileDialog.consumeSavedSimulationPath();
        if (savedSimulationPath.parent_path().lexically_normal() == scenesDirectory_.lexically_normal()) {
            pendingReloadFrames_ = 1;
        }
    }
    // Задержка, что бы изображение успело записаться в файл сохранения .lat
    if (pendingReloadFrames_ > 0 && --pendingReloadFrames_ == 0) {
        sceneCatalogLoaded_ = false;
    }

    fileDialog.setSimulationDirectory(scenesDirectory_.string());
    ensureSceneCatalogLoaded();
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

    int recordingFormat = static_cast<int>(recordingFormat_);
    drawIOPanelRecordingFormatCombo("##recording_format", recordingFormat, saveButtonWidth * scale, scale);
    recordingFormat_ = static_cast<RecordingFormat>(recordingFormat);

    const bool videoSelected = recordingFormat == kRecordingFormatVideo;
    const bool xyzSelected = recordingFormat == kRecordingFormatXYZ;
    const bool videoRecording = uiState.captureRecording;
    const bool xyzRecording = uiState.xyzRecording;
    const bool selectedRecording = videoSelected ? videoRecording : xyzRecording;
    const bool selectedFormatAvailable = videoSelected ? uiState.captureAvailable : true;
    const char* captureLabel = selectedRecording ? "Стоп" : "Запись";

    ImGui::SameLine();
    ImGui::BeginDisabled(!selectedFormatAvailable);
    if (ImGui::Button(captureLabel, ImVec2(saveButtonWidth * scale, 0.f))) {
        if (videoSelected) {
            AppSignals::Capture::ToggleRecording.emit();
        }
        else {
            AppSignals::Capture::ToggleXYZRecording.emit();
        }
    }
    ImGui::EndDisabled();

    if (videoSelected) {
        if (uiState.captureAvailable) {
            drawIOPanelCaptureStatus(uiState);
        }
    }
    else if (xyzSelected) {
        drawIOPanelRecordingStatusLine(xyzRecording, uiState.xyzFps, uiState.xyzFrameCount);
    }
    ImGuiTreeNodeFlags generatorHeaderFlags = generatorsExpanded_ ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
    const bool generatorsOpen = ImGui::CollapsingHeader("Генераторы", generatorHeaderFlags);
    generatorsExpanded_ = generatorsOpen;
    if (!generatorsOpen) {
        AppSignals::UI::ClearGeneratorPhantom.emit();
    }
    else {
        if (ImGui::BeginCombo("##generator_kind", generatorLabel(generatorKind_))) {
            constexpr IOPanel::GeneratorKind generators[] = {
                IOPanel::GeneratorKind::TriangularBipyramid,
                IOPanel::GeneratorKind::RandomFill,
                IOPanel::GeneratorKind::LatticeFill,
            };
            for (IOPanel::GeneratorKind kind : generators) {
                const bool selected = kind == generatorKind_;
                if (ImGui::Selectable(generatorLabel(kind), selected)) {
                    generatorKind_ = kind;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        bool createRequested = false;
        switch (generatorKind_) {
    case GeneratorKind::TriangularBipyramid:
        ImGui::SliderInt("##atoms_per_axis_tbp", &generatorAxisCount_, 2, 100);
        ImGui::SameLine();
        drawIOPanelAtomTypeCombo("##atom_type_tbp", tbpAtomType_, 80.f * scale, scale);
        createRequested = ImGui::Button("Create##tbp", ImVec2(buttonWidth * scale, 0.f));
        if (createRequested) {
            AppSignals::UI::CreateTriangularBipyramidCrystal.emit(generatorAxisCount_, tbpAtomType_);
        }
        break;
    case GeneratorKind::RandomFill: {
        drawRegionEditor("random_fill_region", randomFillRegion_, scale);
        AppSignals::UI::SetGeneratorPhantom.emit(randomFillRegion_);
        drawCompositionEditor("random_fill_composition", randomFillComposition_, scale, "Ar");

        if (ImGui::BeginCombo("Mode##random_fill", spawnModeLabel(randomFillOptions_.mode))) {
            constexpr Generators::SpawnMode spawnModes[] = {
                Generators::SpawnMode::Reset,
                Generators::SpawnMode::Add,
                Generators::SpawnMode::Replace,
            };
            for (Generators::SpawnMode value : spawnModes) {
                const bool selected = value == randomFillOptions_.mode;
                if (ImGui::Selectable(spawnModeLabel(value), selected)) {
                    randomFillOptions_.mode = value;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(120.0f * scale);
        ImGui::DragFloat("Density##random_fill", &randomFillOptions_.density, 0.001f, 0.0f, 10.0f, "%.4f");
        ImGui::SetNextItemWidth(120.0f * scale);
        ImGui::DragFloat("Temperature##random_fill", &randomFillOptions_.temperature, 1.0f, 0.0f, 100000.0f, "%.1f");

        int maxAttemptsPerSpawn = static_cast<int>(randomFillOptions_.maxAttemptsPerSpawn);
        ImGui::SetNextItemWidth(120.0f * scale);
        if (ImGui::DragInt("Attempts##random_fill", &maxAttemptsPerSpawn, 1.0f, 1, INT_MAX, "%d")) {
            randomFillOptions_.maxAttemptsPerSpawn = static_cast<uint32_t>(std::max(maxAttemptsPerSpawn, 1));
        }

        int seed = static_cast<int>(randomFillOptions_.seed);
        ImGui::SetNextItemWidth(120.0f * scale);
        if (ImGui::DragInt("Seed##random_fill", &seed, 1.0f, 0, INT_MAX, "%d")) {
            randomFillOptions_.seed = static_cast<uint32_t>(std::max(seed, 0));
        }

        ImGui::Checkbox("Random rotation##random_fill", &randomFillOptions_.randomRotation);
        ImGui::SameLine();
        ImGui::Checkbox("Fixed##random_fill", &randomFillOptions_.fixed);

        createRequested = ImGui::Button("Create##random_fill", ImVec2(buttonWidth * scale, 0.f));
        if (createRequested) {
            AppSignals::UI::RandomFillRequest request;
            request.region = randomFillRegion_;
            request.composition = randomFillComposition_;
            request.options = randomFillOptions_;
            AppSignals::UI::CreateRandomFill.emit(request);
        }
        break;
    }
    case GeneratorKind::LatticeFill: {
        drawRegionEditor("lattice_fill_region", latticeFillRegion_, scale);
        AppSignals::UI::SetGeneratorPhantom.emit(latticeFillRegion_);

        if (ImGui::BeginCombo("Structure##lattice_fill", latticeStructureLabel(latticeFillOptions_.structure))) {
            constexpr Generators::LatticeStructure structures[] = {
                Generators::LatticeStructure::Bcc,
                Generators::LatticeStructure::Hex,
            };
            for (Generators::LatticeStructure value : structures) {
                const bool selected = value == latticeFillOptions_.structure;
                if (ImGui::Selectable(latticeStructureLabel(value), selected)) {
                    latticeFillOptions_.structure = value;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        drawCompositionEditor("lattice_fill_composition", latticeFillComposition_, scale, "Z");

        if (ImGui::BeginCombo("Mode##lattice_fill", spawnModeLabel(latticeFillOptions_.mode))) {
            constexpr Generators::SpawnMode spawnModes[] = {
                Generators::SpawnMode::Reset,
                Generators::SpawnMode::Add,
                Generators::SpawnMode::Replace,
            };
            for (Generators::SpawnMode value : spawnModes) {
                const bool selected = value == latticeFillOptions_.mode;
                if (ImGui::Selectable(spawnModeLabel(value), selected)) {
                    latticeFillOptions_.mode = value;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        int seed = static_cast<int>(latticeFillOptions_.seed);
        ImGui::SetNextItemWidth(120.0f * scale);
        if (ImGui::DragInt("Seed##lattice_fill", &seed, 1.0f, 0, INT_MAX, "%d")) {
            latticeFillOptions_.seed = static_cast<uint32_t>(std::max(seed, 0));
        }
        ImGui::Checkbox("Fixed##lattice_fill", &latticeFillOptions_.fixed);

        createRequested = ImGui::Button("Create##lattice_fill", ImVec2(buttonWidth * scale, 0.f));
        if (createRequested) {
            AppSignals::UI::LatticeFillRequest request;
            request.region = latticeFillRegion_;
            request.composition = latticeFillComposition_;
            request.options = latticeFillOptions_;
            AppSignals::UI::CreateLatticeFill.emit(request);
        }
        break;
    }
    default:
        AppSignals::UI::ClearGeneratorPhantom.emit();
        break;
    }
    }

    ImGui::SeparatorText("Сцены");
    std::array<char, 512> scenesDirBuffer{};
    const std::string scenesDir = scenesDirectory_.string();
    std::snprintf(scenesDirBuffer.data(), scenesDirBuffer.size(), "%s", scenesDir.data());
    const float browseButtonWidth = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseButtonWidth - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##scenes_dir", scenesDirBuffer.data(), scenesDirBuffer.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("...##scenes_dir_browse", ImVec2(browseButtonWidth, 0.0f))) {
        fileDialog.openSceneDirectory(scenesDir);
    }

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float tileSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float tileWidth = std::max(80.0f, (availableWidth - tileSpacing) * 0.5f);
    const float tileHeight = tileWidth * 0.78f;
    const ImVec2 previewSize(tileWidth, tileHeight);
    bool openDeleteConfirmation = false;
    constexpr const char* kDeletePopupId = "DeleteSceneConfirm";

    for (size_t i = 0; i < sceneTiles_.size();) {
        IOPanelSceneTile& tile = sceneTiles_[i];
        ImGui::PushID(static_cast<int>(i));

        if (i % 2 != 0) {
            ImGui::SameLine();
        }

        ImGui::InvisibleButton("scene_tile", ImVec2(tileWidth, tileHeight));
        const ImVec2 tileMin = ImGui::GetItemRectMin();
        const ImVec2 tileMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(tileMin, tileMax, ImGui::GetColorU32(ImVec4(0.14f, 0.17f, 0.21f, 1.0f)), kSceneTileRounding);

        const bool isHovered = ImGui::IsItemHovered();

        if (tile.hasPreview) {
            const ImTextureID textureId = (ImTextureID)(WGPUTextureView)*tile.previewTextureView;
            const glm::ivec2 textureSize(static_cast<int>(tile.previewSize.x), static_cast<int>(tile.previewSize.y));
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
            drawIOPanelScenePreviewFallback(previewSize);
        }

        const ImVec4 borderColor = isHovered ? ImVec4(0.38f, 0.64f, 1.00f, 1.0f) : ImVec4(0.30f, 0.36f, 0.42f, 1.0f);
        drawList->AddRect(tileMin, tileMax, ImGui::GetColorU32(borderColor), kSceneTileRounding, 0, isHovered ? 1.5f : 1.0f);

        const float deleteButtonSize = 20.0f * scale;
        const float deleteButtonPadding = 8.0f * scale;
        const ImVec2 deleteMin(tileMax.x - deleteButtonSize - deleteButtonPadding, tileMin.y + deleteButtonPadding);
        const ImVec2 deleteMax(deleteMin.x + deleteButtonSize, deleteMin.y + deleteButtonSize);
        const bool deleteHovered = ImGui::IsMouseHoveringRect(deleteMin, deleteMax);
        const bool deleteClicked = deleteHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        drawList->AddRectFilled(deleteMin, deleteMax,
                                ImGui::GetColorU32(deleteHovered ? ImVec4(0.76f, 0.24f, 0.28f, 0.96f) : ImVec4(0.10f, 0.12f, 0.16f, 0.86f)),
                                6.0f * scale);
        const ImVec2 deleteCenter(deleteMin.x + deleteButtonSize * 0.5f - 0.5f * scale,
                                  deleteMin.y + deleteButtonSize * 0.5f - 0.5f * scale);
        const float crossHalfExtent = deleteButtonSize * 0.18f;
        const ImU32 crossColor = ImGui::GetColorU32(ImVec4(0.96f, 0.97f, 0.99f, 1.0f));
        drawList->AddLine(ImVec2(deleteCenter.x - crossHalfExtent, deleteCenter.y - crossHalfExtent),
                          ImVec2(deleteCenter.x + crossHalfExtent, deleteCenter.y + crossHalfExtent), crossColor, 1.4f * scale);
        drawList->AddLine(ImVec2(deleteCenter.x - crossHalfExtent, deleteCenter.y + crossHalfExtent),
                          ImVec2(deleteCenter.x + crossHalfExtent, deleteCenter.y - crossHalfExtent), crossColor, 1.4f * scale);

        if (!deleteHovered && isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            AppSignals::UI::LoadSimulation.emit(tile.path);
        }

        if (deleteClicked) {
            pendingDeleteScenePath_ = tile.path;
            pendingDeleteSceneTitle_ = tile.title;
            pendingDeleteError_.clear();
            pendingDeletePopupPos_ = ImVec2(deleteMax.x + 8.0f * scale, deleteMin.y);
            openDeleteConfirmation = true;
        }

        const ImVec2 titlePos(tileMin.x + 10.0f, tileMax.y - 25.0f);
        drawList->AddText(ImVec2(titlePos.x + 1.0f, titlePos.y + 1.0f), ImGui::GetColorU32(ImVec4(0.02f, 0.03f, 0.05f, 0.85f)),
                          tile.title.data());
        drawList->AddText(titlePos, ImGui::GetColorU32(ImVec4(0.95f, 0.96f, 0.98f, 1.0f)), tile.title.data());
        if (!tile.description.empty()) {
            const ImVec2 descriptionPos(tileMin.x + 10.0f, tileMax.y - 15.0f);
            drawList->AddText(ImVec2(descriptionPos.x + 1.0f, descriptionPos.y + 1.0f),
                              ImGui::GetColorU32(ImVec4(0.02f, 0.03f, 0.05f, 0.80f)), tile.description.data());
            drawList->AddText(descriptionPos, ImGui::GetColorU32(ImVec4(0.82f, 0.86f, 0.90f, 0.98f)), tile.description.data());
        }

        ImGui::PopID();
        ++i;
    }

    if (openDeleteConfirmation) {
        ImGui::OpenPopup(kDeletePopupId);
    }

    if (!pendingDeleteScenePath_.empty()) {
        ImGui::SetNextWindowPos(pendingDeletePopupPos_, ImGuiCond_Appearing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * scale, 10.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f * scale);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.10f, 0.13f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.33f, 0.38f, 0.46f, 0.95f));

        if (ImGui::BeginPopup(kDeletePopupId,
                              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            const char* deleteTitle = "Удалить сцену?";
            const float titleWidth = ImGui::CalcTextSize(deleteTitle).x;
            const float titleOffsetX = std::max(0.0f, (ImGui::GetContentRegionAvail().x - titleWidth) * 0.5f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + titleOffsetX);
            ImGui::TextUnformatted(deleteTitle);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.17f, 0.92f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.77f, 0.20f, 0.24f, 0.98f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.58f, 0.14f, 0.18f, 1.0f));
            if (ImGui::Button("Удалить", ImVec2(100.0f * scale, 0.0f))) {
                const std::string deletedScenePath = pendingDeleteScenePath_;
                std::error_code removeError;
                const bool removed = std::filesystem::remove(deletedScenePath, removeError);
                if (!removeError && (removed || !std::filesystem::exists(deletedScenePath))) {
                    removeSceneTileByPath(deletedScenePath);
                    clearPendingDeleteState();
                    ImGui::CloseCurrentPopup();
                }
                else {
                    pendingDeleteError_ = removeError ? removeError.message() : "Не удалось удалить файл сцены";
                }
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Отмена", ImVec2(100.0f * scale, 0.0f))) {
                clearPendingDeleteState();
                ImGui::CloseCurrentPopup();
            }

            if (!pendingDeleteError_.empty()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.48f, 0.48f, 1.0f));
                ImGui::TextWrapped("%s", pendingDeleteError_.data());
                ImGui::PopStyleColor();
            }

            ImGui::EndPopup();
        }

        if (!ImGui::IsPopupOpen(kDeletePopupId)) {
            clearPendingDeleteState();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }

    ImGui::End();
}

bool IOPanel::canSpawnFromRegionTool() const {
    return generatorKind_ == GeneratorKind::RandomFill || generatorKind_ == GeneratorKind::LatticeFill;
}

bool IOPanel::emitSpawnFromRegion(const AppSignals::UI::GeneratorRegionSpec& region) const {
    if (generatorKind_ == GeneratorKind::RandomFill) {
        AppSignals::UI::RandomFillRequest request;
        request.region = region;
        request.composition = randomFillComposition_;
        request.options = randomFillOptions_;
        AppSignals::UI::CreateRandomFill.emit(request);
        return true;
    }

    if (generatorKind_ == GeneratorKind::LatticeFill) {
        AppSignals::UI::LatticeFillRequest request;
        request.region = region;
        request.composition = latticeFillComposition_;
        request.options = latticeFillOptions_;
        AppSignals::UI::CreateLatticeFill.emit(request);
        return true;
    }

    return false;
}
