#include "RendererCapture.h"

#include <algorithm>

#include "Engine/metrics/Profiler.h"
#include <glad/glad.h>

RendererCapture::~RendererCapture() {
    releasePBOs();
}

CapturedFrame RendererCapture::captureRGBA_PBO(sf::RenderTarget& target, bool flipVertically) {
    PROFILE_SCOPE("Capture::readback");

    CapturedFrame frame;
    const sf::Vector2u size = target.getSize();
    if (size.x == 0 || size.y == 0) {
        return frame;
    }

    if (!target.setActive(true)) {
        return frame;
    }

    ensurePBOs(size.x, size.y);
    if (byteSize_ == 0) {
        return frame;
    }

    GLint previousPackAlignment = 4;
    GLint previousPackBuffer = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos_[static_cast<size_t>(writeIndex_)]);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (primed_) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos_[static_cast<size_t>(readIndex_)]);
        void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, static_cast<GLsizeiptr>(byteSize_), GL_MAP_READ_BIT);
        if (mapped != nullptr) {
            frame.width = width_;
            frame.height = height_;
            frame.rgba.resize(byteSize_);
            std::copy_n(static_cast<const uint8_t*>(mapped), byteSize_, frame.rgba.data());
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            if (flipVertically) {
                flipRows(frame);
            }
        }
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, previousPackBuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);

    std::swap(writeIndex_, readIndex_);
    primed_ = true;
    return frame;
}

CapturedFrame RendererCapture::consumePendingFrame(bool flipVertically) {
    PROFILE_SCOPE("Capture::readback");

    CapturedFrame frame;
    if (!primed_ || pbos_[static_cast<size_t>(readIndex_)] == 0 || byteSize_ == 0) {
        return frame;
    }

    GLint previousPackAlignment = 4;
    GLint previousPackBuffer = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos_[static_cast<size_t>(readIndex_)]);
    void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, static_cast<GLsizeiptr>(byteSize_), GL_MAP_READ_BIT);
    if (mapped != nullptr) {
        frame.width = width_;
        frame.height = height_;
        frame.rgba.resize(byteSize_);
        std::copy_n(static_cast<const uint8_t*>(mapped), byteSize_, frame.rgba.data());
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

        if (flipVertically) {
            flipRows(frame);
        }
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, previousPackBuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
    primed_ = false;
    return frame;
}

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

void RendererCapture::ensurePBOs(uint32_t width, uint32_t height) {
    const size_t requiredByteSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (pbos_[0] != 0 && width_ == width && height_ == height && byteSize_ == requiredByteSize) {
        return;
    }

    releasePBOs();

    width_ = width;
    height_ = height;
    byteSize_ = requiredByteSize;
    writeIndex_ = 0;
    readIndex_ = 1;
    primed_ = false;

    glGenBuffers(2, pbos_.data());
    for (const unsigned int pbo : pbos_) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(byteSize_), nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void RendererCapture::releasePBOs() {
    if (pbos_[0] != 0 || pbos_[1] != 0) {
        glDeleteBuffers(2, pbos_.data());
    }
    pbos_ = {};
    width_ = 0;
    height_ = 0;
    byteSize_ = 0;
    writeIndex_ = 0;
    readIndex_ = 1;
    primed_ = false;
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
