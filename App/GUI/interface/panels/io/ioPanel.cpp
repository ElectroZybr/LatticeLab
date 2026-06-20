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
#include "GUI/interface/style/ComboStyle.h"

namespace {
    constexpr float kSceneTileRounding = 10.0f;
    constexpr int kRecordingFormatVideo = static_cast<int>(IOPanel::RecordingFormat::MP4);
    constexpr int kRecordingFormatXYZ = static_cast<int>(IOPanel::RecordingFormat::XYZ);
    constexpr float kCompactPopupWidth = 224.0f;
    constexpr float kCompactFieldWidth = 126.0f;
    const std::filesystem::path kBaseMoleculesDirectory = std::filesystem::path("Mods") / "Base" / "Molecules";

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

    std::string speciesLabel(std::string_view species) {
        if (species == "Z") {
            return "Zerium";
        }
        return std::string(species);
    }

    std::vector<std::string> listAvailableMolecules() {
        std::vector<std::string> moleculeNames;
        if (!std::filesystem::exists(kBaseMoleculesDirectory) || !std::filesystem::is_directory(kBaseMoleculesDirectory)) {
            return moleculeNames;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(kBaseMoleculesDirectory)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".pdb") {
                continue;
            }

            const std::string moleculeName = entry.path().stem().string();
            if (!moleculeName.empty()) {
                moleculeNames.push_back(moleculeName);
            }
        }

        std::sort(moleculeNames.begin(), moleculeNames.end());
        moleculeNames.erase(std::unique(moleculeNames.begin(), moleculeNames.end()), moleculeNames.end());
        return moleculeNames;
    }

    void drawSpeciesCombo(const char* id, std::string& species, float width, float scale, const std::vector<std::string>& moleculeNames,
                          bool allowMolecules) {
        const std::string previewLabel = speciesLabel(species);
        if (ComboStyle::beginCombo(id, previewLabel.c_str(), width, scale)) {
            if (allowMolecules && !moleculeNames.empty()) {
                ImGui::SeparatorText("Molecules");
                for (const std::string& moleculeName : moleculeNames) {
                    const bool selected = (species == moleculeName);
                    if (ImGui::Selectable(moleculeName.c_str(), selected)) {
                        species = moleculeName;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }

            ImGui::SeparatorText("Atoms");
            for (int i = 0; i < static_cast<int>(AtomData::Type::COUNT); ++i) {
                const AtomData::Type atomType = static_cast<AtomData::Type>(i);
                const std::string_view symbol = AtomData::symbol(atomType);
                const std::string candidateLabel = speciesLabel(symbol);
                const bool selected = (species == symbol);
                if (ImGui::Selectable(candidateLabel.c_str(), selected)) {
                    species = std::string(symbol);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    bool compactComboRow(const char* label, const char* id, const char* preview, float scale) {
        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        const bool open = ComboStyle::beginCombo(id, preview, 0.0f, scale);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        return open;
    }

    bool compactSliderFloatRow(const char* label, const char* id, float& value, float minValue, float maxValue, const char* format,
                               ImGuiSliderFlags flags, float scale) {
        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        const bool changed = ImGui::SliderFloat(id, &value, minValue, maxValue, format, flags);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        return changed;
    }

    bool compactDragFloatRow(const char* label, const char* id, float& value, float speed, float minValue, float maxValue, const char* format,
                             float scale) {
        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        const bool changed = ImGui::DragFloat(id, &value, speed, minValue, maxValue, format);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        return changed;
    }

    bool compactDragIntRow(const char* label, const char* id, int& value, float speed, int minValue, int maxValue, const char* format,
                           float scale) {
        ImGui::SetNextItemWidth(kCompactFieldWidth * scale);
        const bool changed = ImGui::DragInt(id, &value, speed, minValue, maxValue, format);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        return changed;
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

    void drawCompositionEditor(const char* prefix, std::vector<AppSignals::UI::GeneratorComposeSpec>& composition, float scale,
                               const char* defaultSpecies, const std::vector<std::string>& moleculeNames, bool allowMolecules,
                               bool compact = false) {
        const float speciesWidth = std::floor((compact ? 72.0f : 100.0f) * scale);
        const float fractionWidth = std::floor((compact ? 54.0f : 72.0f) * scale);
        const float removeWidth = std::floor((compact ? 18.0f : 22.0f) * scale);
        float totalFraction = 0.0f;
        for (size_t i = 0; i < composition.size(); ++i) {
            AppSignals::UI::GeneratorComposeSpec& entry = composition[i];
            totalFraction += std::max(entry.fraction, 0.0f);

            ImGui::PushID(static_cast<int>(i));
            drawSpeciesCombo((std::string("##species_") + prefix).c_str(), entry.species, speciesWidth, scale, moleculeNames, allowMolecules);
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

        if (ImGui::Button((std::string("Add type##") + prefix).c_str(), ImVec2((compact ? 86.0f : 140.0f) * scale, 0.0f))) {
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

void IOPanel::drawRandomFillGeneratorEditor(float scale, const std::vector<std::string>& availableMolecules,
                                            AppSignals::UI::GeneratorRegionSpec* regionOverride,
                                            std::vector<AppSignals::UI::GeneratorComposeSpec>& composition,
                                            Generators::RandomFillOptions& options, bool showRegionEditor, bool showCreateButton,
                                            bool compact) {
    AppSignals::UI::GeneratorRegionSpec& region = regionOverride != nullptr ? *regionOverride : randomFillRegion_;
    if (showRegionEditor) {
        drawRegionEditor(regionOverride != nullptr ? "region_tool_random_fill" : "random_fill_region", region, scale);
        AppSignals::UI::SetGeneratorPhantom.emit(region);
    }
    drawCompositionEditor(regionOverride != nullptr ? "region_tool_random_fill_composition" : "random_fill_composition", composition, scale, "Z",
                          availableMolecules, true, compact);

    const char* modeId = regionOverride != nullptr ? "##region_tool_random_fill_mode" : "##random_fill_mode";
    const bool modeOpen = compact ? compactComboRow("Mode", modeId, spawnModeLabel(options.mode), scale)
                                  : ComboStyle::beginCombo(regionOverride != nullptr ? "Mode##region_tool_random_fill" : "Mode##random_fill",
                                                           spawnModeLabel(options.mode), 0.0f, scale);
    if (modeOpen) {
        constexpr Generators::SpawnMode spawnModes[] = {
            Generators::SpawnMode::Reset,
            Generators::SpawnMode::Add,
            Generators::SpawnMode::Replace,
        };
        for (Generators::SpawnMode value : spawnModes) {
            const bool selected = value == options.mode;
            if (ImGui::Selectable(spawnModeLabel(value), selected)) {
                options.mode = value;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (compact) {
        compactDragFloatRow("Density", "##region_tool_random_fill_density_compact", options.density, 0.001f, 0.0f, 10.0f, "%.4f", scale);
        compactDragFloatRow("Temp", "##region_tool_random_fill_temperature_compact", options.temperature, 1.0f, 0.0f, 100000.0f, "%.1f",
                            scale);
    }
    else {
        ImGui::SetNextItemWidth(120.0f * scale);
        ImGui::DragFloat(regionOverride != nullptr ? "Density##region_tool_random_fill" : "Density##random_fill", &options.density, 0.001f, 0.0f,
                         10.0f, "%.4f");
        ImGui::SetNextItemWidth(120.0f * scale);
        ImGui::DragFloat(regionOverride != nullptr ? "Temperature##region_tool_random_fill" : "Temperature##random_fill", &options.temperature,
                         1.0f, 0.0f, 100000.0f, "%.1f");
    }

    if (!compact) {
        int maxAttemptsPerSpawn = static_cast<int>(options.maxAttemptsPerSpawn);
        if ((ImGui::SetNextItemWidth(120.0f * scale),
             ImGui::DragInt(regionOverride != nullptr ? "Attempts##region_tool_random_fill" : "Attempts##random_fill", &maxAttemptsPerSpawn,
                            1.0f, 1, INT_MAX, "%d"))) {
            options.maxAttemptsPerSpawn = static_cast<uint32_t>(std::max(maxAttemptsPerSpawn, 1));
        }

        int seed = static_cast<int>(options.seed);
        if ((ImGui::SetNextItemWidth(120.0f * scale),
             ImGui::DragInt(regionOverride != nullptr ? "Seed##region_tool_random_fill" : "Seed##random_fill", &seed, 1.0f, 0, INT_MAX, "%d"))) {
            options.seed = static_cast<uint32_t>(std::max(seed, 0));
        }
    }

    ImGui::Checkbox(regionOverride != nullptr ? "Random rotation##region_tool_random_fill" : "Random rotation##random_fill", &options.randomRotation);
    ImGui::SameLine();
    ImGui::Checkbox(regionOverride != nullptr ? "Fixed##region_tool_random_fill" : "Fixed##random_fill", &options.fixed);

    if (showCreateButton) {
        if (ImGui::Button("Create##random_fill", ImVec2(140.0f * scale, 0.0f))) {
            AppSignals::UI::RandomFillRequest request;
            request.region = region;
            request.composition = composition;
            request.options = options;
            AppSignals::UI::CreateRandomFill.emit(request);
        }
    }
}

void IOPanel::drawLatticeFillGeneratorEditor(float scale, const std::vector<std::string>& availableMolecules,
                                             AppSignals::UI::GeneratorRegionSpec* regionOverride,
                                             std::vector<AppSignals::UI::GeneratorComposeSpec>& composition,
                                             Generators::LatticeFillOptions& options, bool showRegionEditor, bool showCreateButton,
                                             bool compact) {
    AppSignals::UI::GeneratorRegionSpec& region = regionOverride != nullptr ? *regionOverride : latticeFillRegion_;
    if (showRegionEditor) {
        drawRegionEditor(regionOverride != nullptr ? "region_tool_lattice_fill" : "lattice_fill_region", region, scale);
        AppSignals::UI::SetGeneratorPhantom.emit(region);
    }

    const char* structureId = regionOverride != nullptr ? "##region_tool_lattice_fill_structure" : "##lattice_fill_structure";
    const bool structureOpen = compact ? compactComboRow("Type", structureId, latticeStructureLabel(options.structure), scale)
                                       : ComboStyle::beginCombo(regionOverride != nullptr ? "Structure##region_tool_lattice_fill"
                                                                                          : "Structure##lattice_fill",
                                                                latticeStructureLabel(options.structure), 0.0f, scale);
    if (structureOpen) {
        constexpr Generators::LatticeStructure structures[] = {
            Generators::LatticeStructure::Bcc,
            Generators::LatticeStructure::Hex,
        };
        for (Generators::LatticeStructure value : structures) {
            const bool selected = value == options.structure;
            if (ImGui::Selectable(latticeStructureLabel(value), selected)) {
                options.structure = value;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    drawCompositionEditor(regionOverride != nullptr ? "region_tool_lattice_fill_composition" : "lattice_fill_composition", composition, scale, "Z",
                          availableMolecules, false, compact);

    const char* modeId = regionOverride != nullptr ? "##region_tool_lattice_fill_mode" : "##lattice_fill_mode";
    const bool modeOpen = compact ? compactComboRow("Mode", modeId, spawnModeLabel(options.mode), scale)
                                  : ComboStyle::beginCombo(regionOverride != nullptr ? "Mode##region_tool_lattice_fill"
                                                                                     : "Mode##lattice_fill",
                                                           spawnModeLabel(options.mode), 0.0f, scale);
    if (modeOpen) {
        constexpr Generators::SpawnMode spawnModes[] = {
            Generators::SpawnMode::Reset,
            Generators::SpawnMode::Add,
            Generators::SpawnMode::Replace,
        };
        for (Generators::SpawnMode value : spawnModes) {
            const bool selected = value == options.mode;
            if (ImGui::Selectable(spawnModeLabel(value), selected)) {
                options.mode = value;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (!compact) {
        int seed = static_cast<int>(options.seed);
        if ((ImGui::SetNextItemWidth(120.0f * scale),
             ImGui::DragInt(regionOverride != nullptr ? "Seed##region_tool_lattice_fill" : "Seed##lattice_fill", &seed, 1.0f, 0, INT_MAX,
                            "%d"))) {
            options.seed = static_cast<uint32_t>(std::max(seed, 0));
        }
    }
    ImGui::Checkbox(regionOverride != nullptr ? "Fixed##region_tool_lattice_fill" : "Fixed##lattice_fill", &options.fixed);

    if (showCreateButton) {
        if (ImGui::Button("Create##lattice_fill", ImVec2(140.0f * scale, 0.0f))) {
            AppSignals::UI::LatticeFillRequest request;
            request.region = region;
            request.composition = composition;
            request.options = options;
            AppSignals::UI::CreateLatticeFill.emit(request);
        }
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
    const std::vector<std::string> availableMolecules = listAvailableMolecules();
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

        switch (generatorKind_) {
        case GeneratorKind::TriangularBipyramid:
            ImGui::SliderInt("##atoms_per_axis_tbp", &generatorAxisCount_, 2, 100);
            ImGui::SameLine();
            drawIOPanelAtomTypeCombo("##atom_type_tbp", tbpAtomType_, 80.f * scale, scale);
            if (ImGui::Button("Create##tbp", ImVec2(buttonWidth * scale, 0.f))) {
                AppSignals::UI::CreateTriangularBipyramidCrystal.emit(generatorAxisCount_, tbpAtomType_);
            }
            break;
        case GeneratorKind::RandomFill:
            drawRandomFillGeneratorEditor(scale, availableMolecules, nullptr, randomFillComposition_, randomFillOptions_, true, true, false);
            break;
        case GeneratorKind::LatticeFill:
            drawLatticeFillGeneratorEditor(scale, availableMolecules, nullptr, latticeFillComposition_, latticeFillOptions_, true, true, false);
            break;
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

void IOPanel::drawRegionSpawnPopup(float scale, ImVec2 anchorPos, std::string_view popupId) {
    const float popupWidth = kCompactPopupWidth * scale;
    const float popupHeight = 0.0f;

    ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 10.0f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.35f, 0.44f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.13f, 0.17f, 0.96f));

    if (ImGui::Begin(popupId.data(), nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Spawn region");
        drawRegionSpawnSettings(scale, true);
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void IOPanel::drawRegionSpawnSettings(float scale, bool compact) {
    const std::vector<std::string> availableMolecules = listAvailableMolecules();
    const bool generatorOpen = compact ? compactComboRow("Spawn", "##region_tool_generator_kind", generatorLabel(regionToolGeneratorKind_), scale)
                                       : ComboStyle::beginCombo("##region_tool_generator_kind", generatorLabel(regionToolGeneratorKind_), 0.0f,
                                                                scale);
    if (generatorOpen) {
        constexpr GeneratorKind generators[] = {
            GeneratorKind::RandomFill,
            GeneratorKind::LatticeFill,
        };
        for (GeneratorKind kind : generators) {
            const bool selected = kind == regionToolGeneratorKind_;
            if (ImGui::Selectable(generatorLabel(kind), selected)) {
                regionToolGeneratorKind_ = kind;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    switch (regionToolGeneratorKind_) {
    case GeneratorKind::RandomFill:
        drawRandomFillGeneratorEditor(scale, availableMolecules, nullptr, regionToolRandomFillComposition_, regionToolRandomFillOptions_, false,
                                      false, compact);
        break;
    case GeneratorKind::LatticeFill:
        drawLatticeFillGeneratorEditor(scale, availableMolecules, nullptr, regionToolLatticeFillComposition_, regionToolLatticeFillOptions_, false,
                                       false, compact);
        break;
    default:
        regionToolGeneratorKind_ = GeneratorKind::RandomFill;
        drawRandomFillGeneratorEditor(scale, availableMolecules, nullptr, regionToolRandomFillComposition_, regionToolRandomFillOptions_, false,
                                      false, compact);
        break;
    }
}

bool IOPanel::canSpawnFromRegionTool() const {
    return true;
}

bool IOPanel::emitSpawnFromRegion(const AppSignals::UI::GeneratorRegionSpec& region) const {
    return emitSpawnFromRegion(region, "Z");
}

bool IOPanel::emitSpawnFromRegion(const AppSignals::UI::GeneratorRegionSpec& region, std::string_view species) const {
    if (regionToolGeneratorKind_ == GeneratorKind::LatticeFill) {
        AppSignals::UI::LatticeFillRequest request;
        request.region = region;
        request.composition = regionToolLatticeFillComposition_;
        if (request.composition.empty()) {
            request.composition = {
                {.species = species.empty() ? std::string("Z") : std::string(species), .fraction = 1.0f},
            };
        }
        request.options = regionToolLatticeFillOptions_;
        AppSignals::UI::CreateLatticeFill.emit(request);
        return true;
    }

    AppSignals::UI::RandomFillRequest request;
    request.region = region;
    request.composition = regionToolRandomFillComposition_;
    if (request.composition.empty()) {
        request.composition = {
            {.species = species.empty() ? std::string("Z") : std::string(species), .fraction = 1.0f},
        };
    }
    request.options = regionToolRandomFillOptions_;
    AppSignals::UI::CreateRandomFill.emit(request);
    return true;
}
