#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>

#include "Rendering/RendererCapture.h"

class FrameRecorder {
public:
    FrameRecorder() = default;
    ~FrameRecorder();

    FrameRecorder(const FrameRecorder&) = delete;
    FrameRecorder& operator=(const FrameRecorder&) = delete;

    void start(const std::filesystem::path& sessionDir);
    void stop();

    bool isRecording() const;
    bool submit(CapturedFrame frame);

    uint64_t savedFrameCount() const;
    uint64_t droppedFrameCount() const;
    size_t pendingFrameCount() const;

private:
    bool openEncoder(const CapturedFrame& frame);
    void closeEncoder();
    static std::filesystem::path makeVideoPath(const std::filesystem::path& sessionDir);
    static std::filesystem::path findFfmpegExecutable();

    mutable std::mutex mutex_;
    std::filesystem::path sessionDir_;
    std::filesystem::path ffmpegPath_;
    void* encoderProcess_ = nullptr;
    void* encoderStdinWrite_ = nullptr;
    bool recording_ = false;
    uint32_t frameWidth_ = 0;
    uint32_t frameHeight_ = 0;
    uint64_t nextFrameIndex_ = 0;
    uint64_t savedFrameCount_ = 0;
};
