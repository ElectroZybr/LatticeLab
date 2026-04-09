#include "CaptureController.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>

#include <SFML/Window/Keyboard.hpp>

CaptureSettings CaptureController::settings() const noexcept { return settings_; }

bool CaptureController::isAvailable() const { return available_; }

void CaptureController::setSettings(const CaptureSettings& settings) noexcept { settings_ = settings; }

std::filesystem::path CaptureController::outputDirectory() const { return outputDirectory_; }

void CaptureController::setOutputDirectory(const std::filesystem::path& path) {
    if (!path.empty()) {
        outputDirectory_ = path;
    }
}

void CaptureController::update(double deltaTime) {
    if (!frameRecorder_.isRecording()) {
        captureFps_ = 0.0f;
        captureRateAccum_ = 0.0;
        captureSubmitAccum_ = 0.0;
        lastCaptureFrameCountSample_ = 0;
        blinkElapsed_ = 0.0;
        return;
    }

    captureRateAccum_ += deltaTime;
    captureSubmitAccum_ += deltaTime;
    blinkElapsed_ += deltaTime;

    if (captureRateAccum_ >= 0.5) {
        const uint64_t currentCaptureFrameCount = frameRecorder_.savedFrameCount();
        const uint64_t deltaFrames = currentCaptureFrameCount - lastCaptureFrameCountSample_;
        captureFps_ = static_cast<float>(deltaFrames / captureRateAccum_);
        lastCaptureFrameCountSample_ = currentCaptureFrameCount;
        captureRateAccum_ = 0.0;
    }
}

void CaptureController::syncUiState(UiState& uiState) const {
    uiState.captureAvailable = isAvailable();
    uiState.captureRecording = isRecording();
    uiState.captureFrameCount = savedFrameCount();
    uiState.captureFps = captureFps();
    uiState.captureBlinkElapsed = blinkElapsed();
}

void CaptureController::handleToggleShortcut(sf::RenderWindow& window) {
    const bool captureKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8);
    if (isAvailable() && captureKeyPressed && !toggleShortcutHeld_) {
        toggle(window);
    }
    toggleShortcutHeld_ = captureKeyPressed;
}

void CaptureController::start() {
    if (!isAvailable()) {
        return;
    }
    activeSessionFps_ = std::max(1, settings_.fps);
    frameRecorder_.start(makeCaptureOutputPath(), settings_);
    resetSessionStats();
}

void CaptureController::stop(sf::RenderWindow& window) {
    if (window.setActive(true)) {
        CapturedFrame pendingFrame = rendererCapture_.consumePendingFrame();
        if (!pendingFrame.empty() && frameRecorder_.isRecording()) {
            frameRecorder_.submit(std::move(pendingFrame));
        }
    }

    frameRecorder_.stop();
    captureFps_ = 0.0f;
    blinkElapsed_ = 0.0;
    toggleShortcutHeld_ = false;
}

void CaptureController::toggle(sf::RenderWindow& window) {
    if (frameRecorder_.isRecording()) {
        stop(window);
    }
    else {
        start();
    }
}

void CaptureController::onFrameRendered(sf::RenderWindow& window) {
    if (!frameRecorder_.isRecording()) {
        return;
    }

    const double frameInterval = 1.0 / static_cast<double>(activeSessionFps_);
    const bool shouldSubmitCaptureFrame = captureSubmitAccum_ >= frameInterval;
    CapturedFrame frame = rendererCapture_.captureRGBA_PBO(window);
    if (shouldSubmitCaptureFrame && !frame.empty()) {
        frameRecorder_.submit(std::move(frame));
        captureSubmitAccum_ -= frameInterval;
    }
}

bool CaptureController::isRecording() const { return frameRecorder_.isRecording(); }

uint64_t CaptureController::savedFrameCount() const { return frameRecorder_.isRecording() ? frameRecorder_.savedFrameCount() : 0; }

float CaptureController::captureFps() const noexcept { return frameRecorder_.isRecording() ? captureFps_ : 0.0f; }

double CaptureController::blinkElapsed() const noexcept { return frameRecorder_.isRecording() ? blinkElapsed_ : 0.0; }

std::filesystem::path CaptureController::makeCaptureOutputPath() const {
    namespace fs = std::filesystem;

    const auto now = std::chrono::system_clock::now();
    const std::string date_str = std::format("{:%Y-%m-%d}", std::chrono::floor<std::chrono::days>(now));
    const std::string prefix = date_str + "_";

    const fs::path captures_dir = outputDirectory_.empty() ? fs::path("captures") : outputDirectory_;

    std::error_code ec;
    fs::create_directories(captures_dir, ec);

    int next_index = 1;

    if (fs::exists(captures_dir, ec)) {
        for (const auto& entry : fs::directory_iterator(captures_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const auto& path = entry.path();
            if (path.extension() != ".mp4") {
                continue;
            }

            const std::string stem = path.stem().string();
            if (stem.starts_with(prefix)) {
                std::string_view suffix(stem.data() + prefix.size(), stem.size() - prefix.size());

                int current_val = 0;
                auto [ptr, parse_ec] = std::from_chars(suffix.data(), suffix.data() + suffix.size(), current_val);

                if (parse_ec == std::errc{}) {
                    next_index = std::max(next_index, current_val + 1);
                }
            }
        }
    }

    return captures_dir / std::format("{}{}.mp4", prefix, next_index);
}

void CaptureController::resetSessionStats() {
    captureRateAccum_ = 0.0;
    captureSubmitAccum_ = 0.0;
    blinkElapsed_ = 0.0;
    lastCaptureFrameCountSample_ = 0;
    captureFps_ = 0.0f;
}
