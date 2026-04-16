#pragma once

#include "FrameRecorder.h"

#include <cstdint>
#include <filesystem>

#include "GUI/interface/UiState.h"

class CaptureController {
public:
    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] CaptureSettings settings() const noexcept;
    void setSettings(const CaptureSettings& settings) noexcept;
    [[nodiscard]] std::filesystem::path outputDirectory() const;
    void setOutputDirectory(const std::filesystem::path& path);

    void update(double deltaTime);
    void syncUiState(UiState& uiState) const;
    void handleToggleShortcut();
    void start();
    void stop();
    void toggle();
    void onFrameRendered();

    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] uint64_t savedFrameCount() const;
    [[nodiscard]] float captureFps() const noexcept;
    [[nodiscard]] double blinkElapsed() const noexcept;

private:
    [[nodiscard]] std::filesystem::path makeCaptureOutputPath() const;
    void resetSessionStats();

    bool activeSessionYflip_ = false;
    bool available_ = FrameRecorder::isAvailable();
    CaptureSettings settings_{};
    std::filesystem::path outputDirectory_ = "captures";
    FrameRecorder frameRecorder_{};
    int activeSessionFps_ = settings_.fps;
    double captureRateAccum_ = 0.0;
    double captureSubmitAccum_ = 0.0;
    double blinkElapsed_ = 0.0;
    uint64_t lastCaptureFrameCountSample_ = 0;
    float captureFps_ = 0.0f;
    bool toggleShortcutHeld_ = false;
};
