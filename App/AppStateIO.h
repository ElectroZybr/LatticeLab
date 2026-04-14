#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace sf {
    class RenderWindow;
}

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
    static void save(const sf::RenderWindow& window, const PreviewFrameRect& previewRect, const Simulation& simulation,
                     const IRenderer& renderer, std::string_view path);
    static void load(Simulation& simulation, IRenderer& renderer, std::string_view path);
};
