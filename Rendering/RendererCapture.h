#pragma once

#include <array>
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
    RendererCapture() = default;
    ~RendererCapture();

    RendererCapture(const RendererCapture&) = delete;
    RendererCapture& operator=(const RendererCapture&) = delete;

    CapturedFrame captureRGBA_PBO(sf::RenderTarget& target, bool flipVertically = true);
    CapturedFrame consumePendingFrame(bool flipVertically = true);

    static CapturedFrame captureRGBA(sf::RenderTarget& target, bool flipVertically = true);

private:
    void ensurePBOs(uint32_t width, uint32_t height);
    void releasePBOs();

    static void flipRows(CapturedFrame& frame);

    std::array<unsigned int, 2> pbos_{};
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    size_t byteSize_ = 0;
    int writeIndex_ = 0;
    int readIndex_ = 1;
    bool primed_ = false;
};
