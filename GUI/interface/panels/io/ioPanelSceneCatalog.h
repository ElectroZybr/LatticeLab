#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics/Texture.hpp>
#include <bgfx/bgfx.h>

#include "Engine/math/Vec2.h"

struct IOPanelSceneTile {
    std::string path;
    std::string title;
    std::string description;
    bgfx::TextureHandle previewTexture = BGFX_INVALID_HANDLE;
    Vec2u previewSize;
    bool hasPreview = false;
};

std::vector<IOPanelSceneTile> loadIOPanelSceneTiles(std::string_view scenesDirectory);
