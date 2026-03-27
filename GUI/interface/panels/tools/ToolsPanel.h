#pragma once
#include <optional>
#include <cstdint>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "imgui.h"

enum class ToolsCommand : uint8_t { ToggleRenderer2D, ToggleRenderer3D, ClearSimulation, SetCameraOrbit, SetCameraFree };

class DebugPanel;
class FileDialogManager;
class SettingsPanel;

class ToolsPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::RenderWindow& window, DebugPanel& debug, FileDialogManager& fileDialog, SettingsPanel& settings);

    std::optional<ToolsCommand> popResult();
private:
    bool is3D = false;
    bool isFree = false;
    std::optional<ToolsCommand> pendingResult;
};
