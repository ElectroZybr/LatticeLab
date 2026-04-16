#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <bgfx/bgfx.h>

struct CapturedFrame {
    uint32_t width = 0;
    uint32_t height = 0;
    bgfx::TextureFormat::Enum format;
    std::vector<std::byte> pixels;
    bool yflip = false;

    bool empty() const { return width == 0 || height == 0 || pixels.empty(); }

    size_t byteSize() const { return pixels.size(); }
};

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
    bool submit(const CapturedFrame& frame);

    uint64_t savedFrameCount() const;
    uint64_t droppedFrameCount() const;
    size_t pendingFrameCount() const;

private:
    void writerLoop();
    bool writeFrame(const CapturedFrame& frame);
    bool openEncoder(const CapturedFrame& frame);
    void closeEncoder();
    static std::filesystem::path findFfmpegExecutable();

    std::thread writerThread_;
    mutable std::mutex queueMutex_;
    std::condition_variable cv_;
    std::queue<CapturedFrame> frameQueue_;
    bool recording_ = false;
    std::atomic<uint64_t> savedFrameCount_ = 0;
    std::atomic<uint64_t> droppedFrameCount_ = 0;

    std::filesystem::path outputPath_;
    std::filesystem::path ffmpegPath_;
    CaptureSettings settings_{};
    void* encoderProcess_ = nullptr;
    void* encoderStdinWrite_ = nullptr;
    uint32_t frameWidth_ = 0;
    uint32_t frameHeight_ = 0;
    uint64_t nextFrameIndex_ = 0;
};
