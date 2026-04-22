#include "CaptureController.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <format>

#include "Engine/metrics/Profiler.h"
#include "GUI/interface/UiState.h"
#include "GUI/io/keyboard/Keyboard.h"

CaptureController::CaptureController() {

#ifdef _WIN32
    available_ = (std::system("ffmpeg -version >NUL 2>&1") == 0);
#else
    available_ = (std::system("ffmpeg -version >/dev/null 2>&1") == 0);
#endif
}

void CaptureController::setSettings(const CaptureSettings& settings) noexcept { settings_ = settings; }

void CaptureController::setOutputDirectory(const std::filesystem::path& path) { outputDirectory_ = path; }

void CaptureController::start() {
    if (!available_ || isRecording()) {
        return;
    }

    std::filesystem::create_directories(outputDirectory_);
    const std::filesystem::path outputPath = makeCaptureOutputPath();
    const char* pixFmt = capture_utils::toInputPixelFormat(activeFormat_);

    if (!streamer_.start(activeWidth_, activeHeight_, pixFmt, settings_, outputPath)) {
        return;
    }

    producer_.startVideoCapture(&streamer_, settings_, measuredRenderFps_);
    resetSessionStats();
}

void CaptureController::stop() {
    if (!isRecording()) {
        return;
    }

    producer_.stopVideoCapture();
    streamer_.stop();
}

void CaptureController::toggle() { isRecording() ? stop() : start(); }

void CaptureController::handleToggleShortcut() {
    const bool captureKeyPressed = Keyboard::isPressed(GLFW_KEY_F8);
    if (isAvailable() && captureKeyPressed && !toggleShortcutHeld_) {
        toggle();
    }
    toggleShortcutHeld_ = captureKeyPressed;
}

void CaptureController::requestScreenshot(ScreenshotCallback callback) { producer_.requestScreenshot(std::move(callback)); }

wgpu::TextureView CaptureController::acquireRenderTarget(wgpu::Texture surfaceTexture) {
    return producer_.acquireRenderTarget(surfaceTexture);
}

void CaptureController::onFrameRendered(wgpu::Texture texture) {
    PROFILE_SCOPE("CaptureController::onFrameRendered");

    activeFormat_ = texture.getFormat();
    activeWidth_ = texture.getWidth();
    activeHeight_ = texture.getHeight();

    producer_.onFrameRendered(texture);
}

void CaptureController::update(double deltaTime) {
    ++renderFrameCount_;
    renderFpsAccum_ += deltaTime;
    if (renderFpsAccum_ >= 1.0) {
        measuredRenderFps_ = static_cast<uint32_t>(renderFrameCount_ / renderFpsAccum_);
        renderFrameCount_ = 0;
        renderFpsAccum_ = 0.0;
        if (isRecording()) {
            producer_.setFrameSkip(measuredRenderFps_, settings_.fps);
        }
    }

    constexpr double kFpsSampleInterval = 1.0;
    captureRateAccum_ += deltaTime;
    if (captureRateAccum_ >= kFpsSampleInterval) {
        const uint64_t current = savedFrameCount();
        const uint64_t framesThisInterval = current - lastFrameCountSample_;
        captureFps_ = static_cast<float>(framesThisInterval / captureRateAccum_);
        lastFrameCountSample_ = current;
        captureRateAccum_ = 0.0;
    }

    if (isRecording()) {
        blinkElapsed_ += deltaTime;
    }
    else {
        blinkElapsed_ = 0.0;
    }
}

void CaptureController::syncUiState(UiState& uiState) const {
    uiState.captureAvailable = available_;
    uiState.captureRecording = isRecording();
    uiState.captureFps = captureFps_;
    uiState.captureBlinkElapsed = blinkElapsed_;
    uiState.captureFrameCount = producer_.capturedFrameCount();
}

std::filesystem::path CaptureController::makeCaptureOutputPath() const {
    const auto now = std::chrono::system_clock::now();
    return outputDirectory_ / std::format("{:%Y-%m-%d_%H-%M-%S}.mp4", now);
}

void CaptureController::resetSessionStats() {
    lastFrameCountSample_ = 0;
    captureRateAccum_ = 0.0;
    captureFps_ = 0.0f;
    blinkElapsed_ = 0.0;
}
