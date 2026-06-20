#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <imgui.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "App/AppPaths.h"
#include "App/AppSignals.h"

#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "GUI/interface/panels/io/ioPanelSceneCatalog.h"

class FileDialogManager;
namespace Lattice {
    class Simulation;
}
struct UiState;

class IOPanel {
public:
    enum class RecordingFormat : uint8_t {
        MP4,
        XYZ,
    };

    enum class GeneratorKind : uint8_t {
        TriangularBipyramid,
        RandomFill,
        LatticeFill,
    };

    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysVerticalScrollbar;

    void draw(float scale, glm::ivec2 windowSize, Lattice::Simulation& simulation, FileDialogManager& fileDialog, UiState& uiState);
    void drawRegionSpawnPopup(float scale, ImVec2 anchorPos, std::string_view popupId = "##region_spawn_tool_popup");
    void drawRegionSpawnSettings(float scale, bool compact = false);
    void setScenesDirectory(std::filesystem::path scenesDirectory);
    [[nodiscard]] const std::filesystem::path& scenesDirectory() const { return scenesDirectory_; }

    void toggle() {
        visible_ = !visible_;
        if (!visible_) {
            AppSignals::UI::ClearGeneratorPhantom.emit();
        }
    }
    void close() {
        visible_ = false;
        AppSignals::UI::ClearGeneratorPhantom.emit();
    }
    [[nodiscard]] bool isVisible() const { return visible_; }
    [[nodiscard]] bool canSpawnFromRegionTool() const;
    bool emitSpawnFromRegion(const AppSignals::UI::GeneratorRegionSpec& region) const;
    bool emitSpawnFromRegion(const AppSignals::UI::GeneratorRegionSpec& region, std::string_view species) const;

private:
    void drawRandomFillGeneratorEditor(float scale, const std::vector<std::string>& availableMolecules,
                                       AppSignals::UI::GeneratorRegionSpec* regionOverride,
                                       std::vector<AppSignals::UI::GeneratorComposeSpec>& composition,
                                       Generators::RandomFillOptions& options, bool showRegionEditor, bool showCreateButton,
                                       bool compact = false);
    void drawLatticeFillGeneratorEditor(float scale, const std::vector<std::string>& availableMolecules,
                                        AppSignals::UI::GeneratorRegionSpec* regionOverride,
                                        std::vector<AppSignals::UI::GeneratorComposeSpec>& composition,
                                        Generators::LatticeFillOptions& options, bool showRegionEditor, bool showCreateButton,
                                        bool compact = false);
    void ensureSceneCatalogLoaded();
    void clearPendingDeleteState();
    void removeSceneTileByPath(std::string_view path);

    uint8_t pendingReloadFrames_ = 0;
    bool visible_ = false;
    bool sceneCatalogLoaded_ = false;
    bool generatorsExpanded_ = true;
    float animProgress_ = 0.f;
    int generatorAxisCount_ = 25;
    GeneratorKind generatorKind_ = GeneratorKind::TriangularBipyramid;

    AtomData::Type tbpAtomType_ = AtomData::Type::Z;
    AppSignals::UI::GeneratorRegionSpec randomFillRegion_{};
    AppSignals::UI::GeneratorRegionSpec latticeFillRegion_{};
    std::vector<AppSignals::UI::GeneratorComposeSpec> randomFillComposition_ = {
        {.species = "Z", .fraction = 1.0f},
    };
    std::vector<AppSignals::UI::GeneratorComposeSpec> latticeFillComposition_ = {
        {.species = "Z", .fraction = 1.0f},
    };
    Generators::RandomFillOptions randomFillOptions_{
        .mode = Generators::SpawnMode::Replace,
        .density = 0.01f,
        .temperature = 0.0f,
        .margin = 2.0f,
        .maxAttemptsPerSpawn = 32,
        .randomRotation = true,
        .fixed = false,
        .seed = 0,
    };
    Generators::LatticeFillOptions latticeFillOptions_{};
    GeneratorKind regionToolGeneratorKind_ = GeneratorKind::RandomFill;
    std::vector<AppSignals::UI::GeneratorComposeSpec> regionToolRandomFillComposition_ = {
        {.species = "Z", .fraction = 1.0f},
    };
    std::vector<AppSignals::UI::GeneratorComposeSpec> regionToolLatticeFillComposition_ = {
        {.species = "Z", .fraction = 1.0f},
    };
    Generators::RandomFillOptions regionToolRandomFillOptions_{
        .mode = Generators::SpawnMode::Replace,
        .density = 0.01f,
        .temperature = 0.0f,
        .margin = 2.0f,
        .maxAttemptsPerSpawn = 32,
        .randomRotation = true,
        .fixed = false,
        .seed = 0,
    };
    Generators::LatticeFillOptions regionToolLatticeFillOptions_{};
    RecordingFormat recordingFormat_ = RecordingFormat::MP4;
    std::filesystem::path scenesDirectory_ = AppPaths::kDefaultScenesDirectory;
    std::vector<IOPanelSceneTile> sceneTiles_;
    std::string pendingDeleteScenePath_;
    std::string pendingDeleteSceneTitle_;
    std::string pendingDeleteError_;
    ImVec2 pendingDeletePopupPos_ = ImVec2(0.0f, 0.0f);
};
