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
    static std::filesystem::path makeFramePath(const std::filesystem::path& sessionDir, uint64_t frameIndex);
    static bool writeBmp(const std::filesystem::path& path, const CapturedFrame& frame);

    mutable std::mutex mutex_;
    std::filesystem::path sessionDir_;
    bool recording_ = false;
    uint64_t nextFrameIndex_ = 0;
    uint64_t savedFrameCount_ = 0;
};
