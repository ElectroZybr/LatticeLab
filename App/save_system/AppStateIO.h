#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

class Simulation;
class IRenderer;
struct PreviewFrameRect;

struct ImageData {
    std::vector<std::uint8_t> pixels; // RGBA
    uint32_t width = 0;
    uint32_t height = 0;
};

class AppStateIO {
public:
    static void save(const PreviewFrameRect& previewRect, const Simulation& simulation, const IRenderer& renderer, std::string_view path);
    static void load(Simulation& simulation, IRenderer& renderer, std::string_view path);

private:
    static void saveText(const PreviewFrameRect& previewRect, const Simulation& simulation, const IRenderer& renderer,
                         std::string_view path);
    static void saveBinary(const PreviewFrameRect& previewRect, const Simulation& simulation, const IRenderer& renderer,
                           std::string_view path);

    static void loadText(Simulation& simulation, IRenderer& renderer, std::string_view path);
    static void loadBinary(Simulation& simulation, IRenderer& renderer, std::string_view path);
};
