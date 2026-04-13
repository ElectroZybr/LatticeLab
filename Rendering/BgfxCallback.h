#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>

#include <bgfx/bgfx.h>

class BgfxCallback : public bgfx::CallbackI {
public:
    using ScreenShotFn = std::function<void(uint32_t width, uint32_t height, const void* data, uint32_t size, bool yflip)>;

    void setScreenShotCallback(ScreenShotFn fn) { screenShotFn_ = std::move(fn); }

    void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum format,
                    const void* data, uint32_t size, bool yflip) override {
        if (screenShotFn_) {
            screenShotFn_(width, height, data, size, yflip);
        }
    }

    void fatal(const char*, uint16_t, bgfx::Fatal::Enum, const char*) override {}
    void traceVargs(const char*, uint16_t, const char*, va_list) override {}
    void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerEnd() override {}
    uint32_t cacheReadSize(uint64_t) override { return 0; }
    bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
    void cacheWrite(uint64_t, const void*, uint32_t) override {}
    void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, uint32_t) override {}

private:
    ScreenShotFn screenShotFn_;
};