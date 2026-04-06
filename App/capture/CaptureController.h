#pragma once

#include <cstdint>
#include <filesystem>

#include <SFML/Graphics/RenderWindow.hpp>

#include "FrameRecorder.h"
#include "Rendering/RendererCapture.h"

class CaptureController {
public:
    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] CaptureSettings settings() const noexcept;
    void setSettings(const CaptureSettings& settings) noexcept;
    [[nodiscard]] std::filesystem::path outputDirectory() const;
    void setOutputDirectory(const std::filesystem::path& path);

    void update(double deltaTime);
    void start();
    void stop(sf::RenderWindow& window);
    void toggle(sf::RenderWindow& window);
    void onFrameRendered(sf::RenderWindow& window);

    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] uint64_t savedFrameCount() const;
    [[nodiscard]] float captureFps() const noexcept;
    [[nodiscard]] double blinkElapsed() const noexcept;

private:
    [[nodiscard]] std::filesystem::path makeCaptureOutputPath() const;
    void resetSessionStats();

    bool available_ = FrameRecorder::isAvailable();
    CaptureSettings settings_{};
    std::filesystem::path outputDirectory_ = "captures";
    FrameRecorder frameRecorder_{};
    RendererCapture rendererCapture_{};
    int activeSessionFps_ = settings_.fps;
    double captureRateAccum_ = 0.0;
    double captureSubmitAccum_ = 0.0;
    double blinkElapsed_ = 0.0;
    uint64_t lastCaptureFrameCountSample_ = 0;
    float captureFps_ = 0.0f;
};
