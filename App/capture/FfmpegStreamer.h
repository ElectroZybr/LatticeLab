#pragma once

#include "CaptureSettings.h"

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

class FFmpegStreamer {
public:
    FFmpegStreamer() = default;
    ~FFmpegStreamer();

    FFmpegStreamer(const FFmpegStreamer&) = delete;
    FFmpegStreamer& operator=(const FFmpegStreamer&) = delete;

    // Запускает процесс ffmpeg и фоновый поток записи.
    // inputPixFmt из capture_utils::toInputPixelFormat
    bool start(uint32_t width, uint32_t height, const char* inputPixFmt, const CaptureSettings& settings,
               const std::filesystem::path& outputPath);

    // Дожидается дрейна очереди, закрывает pipe, join thread.
    void stop();

    // Возвращает false если кадр дропнут из-за backpressure.
    bool pushFrame(std::vector<std::byte> pixels);

    bool isRunning() const { return running_; }

private:
    void writerLoop();
    std::string buildCommand(uint32_t width, uint32_t height, const char* inputPixFmt, const CaptureSettings& settings,
                             const std::filesystem::path& outputPath) const;

    FILE* pipe_ = nullptr;
    std::thread writerThread_;
    bool running_ = false;

    std::deque<std::vector<std::byte>> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

    static constexpr size_t kMaxQueueDepth = 4;
};