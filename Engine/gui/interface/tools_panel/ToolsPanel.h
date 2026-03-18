#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "imgui.h"

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
};