#pragma once

#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <string_view>
#include <vector>

struct IOPanelSceneTile {
    std::string path;
    std::string title;
    std::string description;
    sf::Texture previewTexture;
    bool hasPreview = false;
};

std::vector<IOPanelSceneTile> loadIOPanelSceneTiles(std::string_view scenesDirectory);
