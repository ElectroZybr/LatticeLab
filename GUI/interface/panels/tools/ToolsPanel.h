#pragma once
#include <optional>
#include <cstdint>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "imgui.h"

enum class ToolsCommand : std::uint8_t { ToggleRenderer2D, ToggleRenderer3D };

class DebugPanel;
class FileDialogManager;

class ToolsPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, sf::RenderWindow& window,
              DebugPanel& debug, FileDialogManager& fileDialog);

    std::optional<ToolsCommand> popResult();
private:
    bool is3D = false;
    std::optional<ToolsCommand> pendingResult;
};