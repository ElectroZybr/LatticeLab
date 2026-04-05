#pragma once

#include <cstdint>
#include <filesystem>

#include <SFML/Graphics/RenderWindow.hpp>

#include "FrameRecorder.h"
#include "Rendering/RendererCapture.h"

class CaptureController {
public:
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
    static std::filesystem::path makeCaptureOutputPath();
    void resetSessionStats();

    FrameRecorder frameRecorder_{};
    RendererCapture rendererCapture_{};
    double captureRateAccum_ = 0.0;
    double captureSubmitAccum_ = 0.0;
    double blinkElapsed_ = 0.0;
    uint64_t lastCaptureFrameCountSample_ = 0;
    float captureFps_ = 0.0f;
};
