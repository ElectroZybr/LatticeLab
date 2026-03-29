#pragma once

#include <cstdint>
#include <optional>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "Engine/physics/AtomData.h"

class FileDialogManager;

enum class IOCommand : uint8_t {
    ClearSimulation,
    CreateCrystal,
    CreateGas,
};

class IOPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;

    void draw(float scale, sf::Vector2u windowSize, FileDialogManager& fileDialog);

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

    std::optional<IOCommand> popResult();

private:
    bool visible_ = false;
    float animProgress_ = 0.f;
    int sceneAxisCount_ = 46;
    bool sceneIs3D_ = true;
    int gasAtomCount_ = 10000;
    bool gasIs3D_ = false;
    float gasDensity_ = 1.0f;
    AtomData::Type atomType_ = AtomData::Type::Z;
    AtomData::Type gasAtomType_ = AtomData::Type::Z;
    std::optional<IOCommand> pendingResult_;
};
