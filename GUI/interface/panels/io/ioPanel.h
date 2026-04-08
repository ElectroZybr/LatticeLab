#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>

#include <filesystem>
#include <string>
#include <vector>

#include "Engine/math/Vec3f.h"
#include "Engine/physics/AtomData.h"
#include "GUI/interface/panels/io/ioPanelSceneCatalog.h"

class FileDialogManager;
class Simulation;
struct UiState;

class IOPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    void draw(float scale, sf::Vector2u windowSize, Simulation& simulation, FileDialogManager& fileDialog, UiState& uiState);

    void toggle() { visible_ = !visible_; }
    void close() { visible_ = false; }
    [[nodiscard]] bool isVisible() const { return visible_; }
    [[nodiscard]] int sceneAxisCount() const { return sceneAxisCount_; }
    [[nodiscard]] bool sceneIs3D() const { return sceneIs3D_; }
    [[nodiscard]] int gasAtomCount() const { return gasAtomCount_; }
    [[nodiscard]] bool gasIs3D() const { return gasIs3D_; }
    [[nodiscard]] AtomData::Type atomType() const { return atomType_; }
    [[nodiscard]] AtomData::Type gasAtomType() const { return gasAtomType_; }
    [[nodiscard]] float gasDensity() const { return gasDensity_; }
    [[nodiscard]] Vec3f boxSize() const { return boxSize_; }

private:
    void ensureSceneCatalogLoaded();
    void clearPendingDeleteState();
    void removeSceneTileByPath(std::string_view path);

    bool visible_ = false;
    bool sceneCatalogLoaded_ = false;
    float animProgress_ = 0.f;
    int sceneAxisCount_ = 25;
    bool sceneIs3D_ = true;
    int gasAtomCount_ = 1000;
    bool gasIs3D_ = false;
    float gasDensity_ = 1.0f;
    Vec3f boxSize_ = Vec3f(100.0f, 100.0f, 6.0f);
    AtomData::Type atomType_ = AtomData::Type::Z;
    AtomData::Type gasAtomType_ = AtomData::Type::Z;
    std::filesystem::path scenesDirectory_ = "demo/scenes";
    std::vector<IOPanelSceneTile> sceneTiles_;
    std::string pendingDeleteScenePath_;
    std::string pendingDeleteSceneTitle_;
    std::string pendingDeleteError_;
    ImVec2 pendingDeletePopupPos_ = ImVec2(0.0f, 0.0f);
};
