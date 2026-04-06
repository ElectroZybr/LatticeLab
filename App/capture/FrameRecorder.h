#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>

#include "Rendering/RendererCapture.h"

struct CaptureSettings {
    enum class Preset {
        Ultrafast,
        Veryfast,
        Faster,
        Fast,
        Medium,
    };

    enum class PixelFormat {
        Yuv420p,
        Yuv444p,
    };

    int fps = 30;
    int crf = 20;
    Preset preset = Preset::Veryfast;
    PixelFormat pixelFormat = PixelFormat::Yuv444p;
};

class FrameRecorder {
public:
    FrameRecorder() = default;
    ~FrameRecorder();

    FrameRecorder(const FrameRecorder&) = delete;
    FrameRecorder& operator=(const FrameRecorder&) = delete;

    void start(const std::filesystem::path& outputPath, CaptureSettings settings);
    void stop();

    [[nodiscard]] static bool isAvailable();
    bool isRecording() const;
    bool submit(CapturedFrame frame);

    uint64_t savedFrameCount() const;
    uint64_t droppedFrameCount() const;
    size_t pendingFrameCount() const;

private:
    bool openEncoder(const CapturedFrame& frame);
    void closeEncoder();
    static std::filesystem::path findFfmpegExecutable();

    mutable std::mutex mutex_;
    std::filesystem::path outputPath_;
    std::filesystem::path ffmpegPath_;
    CaptureSettings settings_{};
    void* encoderProcess_ = nullptr;
    void* encoderStdinWrite_ = nullptr;
    bool recording_ = false;
    uint32_t frameWidth_ = 0;
    uint32_t frameHeight_ = 0;
    uint64_t nextFrameIndex_ = 0;
    uint64_t savedFrameCount_ = 0;
};
