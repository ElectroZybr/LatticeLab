#pragma once

#include <cstdint>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>

struct CapturedFrame {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;

    bool empty() const {
        return width == 0 || height == 0 || rgba.empty();
    }

    size_t byteSize() const {
        return rgba.size();
    }
};

class RendererCapture {
public:
    static CapturedFrame captureRGBA(sf::RenderTarget& target, bool flipVertically = true);

private:
    static void flipRows(CapturedFrame& frame);
};
