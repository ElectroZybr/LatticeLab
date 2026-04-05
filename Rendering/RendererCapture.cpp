#include "RendererCapture.h"

#include <algorithm>

#include "Engine/metrics/Profiler.h"
#include <glad/glad.h>

CapturedFrame RendererCapture::captureRGBA(sf::RenderTarget& target, bool flipVertically) {
    PROFILE_SCOPE("Capture::readback");

    CapturedFrame frame;
    const sf::Vector2u size = target.getSize();
    if (size.x == 0 || size.y == 0) {
        return frame;
    }

    if (!target.setActive(true)) {
        return frame;
    }

    frame.width = size.x;
    frame.height = size.y;
    frame.rgba.resize(static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height) * 4);

    GLint previousPackAlignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadBuffer(GL_BACK);
    glReadPixels(
        0, 0,
        static_cast<GLsizei>(frame.width),
        static_cast<GLsizei>(frame.height),
        GL_RGBA, GL_UNSIGNED_BYTE,
        frame.rgba.data()
    );

    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);

    if (flipVertically) {
        flipRows(frame);
    }

    return frame;
}

void RendererCapture::flipRows(CapturedFrame& frame) {
    if (frame.empty()) {
        return;
    }

    const size_t rowSize = static_cast<size_t>(frame.width) * 4;
    std::vector<uint8_t> scratch(rowSize);

    for (uint32_t y = 0; y < frame.height / 2; ++y) {
        uint8_t* top = frame.rgba.data() + static_cast<size_t>(y) * rowSize;
        uint8_t* bottom = frame.rgba.data() + static_cast<size_t>(frame.height - 1 - y) * rowSize;

        std::copy_n(top, rowSize, scratch.data());
        std::copy_n(bottom, rowSize, top);
        std::copy_n(scratch.data(), rowSize, bottom);
    }
}
