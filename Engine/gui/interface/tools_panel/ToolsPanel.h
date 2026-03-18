#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

class DebugPanel;
class FileDialogManager;

class ToolsPanel {
public:
    void draw(float scale, sf::RenderWindow& window,
              DebugPanel& debug, FileDialogManager& fileDialog);
};