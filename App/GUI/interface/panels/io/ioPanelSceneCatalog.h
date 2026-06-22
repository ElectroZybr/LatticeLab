#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <glm/vec2.hpp>

#include <webgpu/webgpu-raii.hpp>

enum class IOPanelSceneSource : uint8_t {
    BuiltIn,
    User,
};

struct IOPanelSceneDirectory {
    std::filesystem::path path;
    IOPanelSceneSource source = IOPanelSceneSource::BuiltIn;
};

struct IOPanelSceneTile {
    std::string path;
    std::string title;
    std::string description;
    IOPanelSceneSource source = IOPanelSceneSource::BuiltIn;
    bool writable = false;
    wgpu::raii::Texture previewTexture;
    wgpu::raii::TextureView previewTextureView;
    glm::uvec2 previewSize;
    bool hasPreview = false;
};

std::vector<IOPanelSceneTile> loadIOPanelSceneTiles(std::span<const IOPanelSceneDirectory> sceneDirectories);
