#pragma once

#include <cstdint>
#include <optional>

#include <SFML/Graphics.hpp>
#include "imgui.h"

class FileDialogManager;

enum class IOCommand : uint8_t {
    ClearSimulation,
    CreateCrystal,
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

    std::optional<IOCommand> popResult();

private:
    bool visible_ = false;
    float animProgress_ = 0.f;
    int sceneAxisCount_ = 46;
    bool sceneIs3D_ = true;
    std::optional<IOCommand> pendingResult_;
};
