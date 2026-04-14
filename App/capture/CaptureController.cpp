#include "CaptureController.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>

#include <SFML/Window/Keyboard.hpp>
#include <bgfx/bgfx.h>

#include "Rendering/BgfxContext.h"

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

void CaptureController::handleToggleShortcut() {
    const bool captureKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8);
    if (isAvailable() && captureKeyPressed && !toggleShortcutHeld_) {
        toggle();
    }
    toggleShortcutHeld_ = captureKeyPressed;
}

void CaptureController::start() {
    if (!isAvailable()) {
        return;
    }
    activeSessionFps_ = std::max(1, settings_.fps);

    BgfxContext::instance().callback().addScreenShotCallback(
        "capture", [this](uint32_t width, uint32_t height, const void* data, uint32_t size, bool yflip) {
            CapturedFrame frame;
            frame.width = width;
            frame.height = height;
            frame.rgba.assign(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);

            if (yflip) {
                const size_t rowSize = width * 4;

                for (uint32_t y = 0; y < height / 2; ++y) {
                    uint8_t* top = frame.rgba.data() + y * rowSize;
                    uint8_t* bottom = frame.rgba.data() + (height - 1 - y) * rowSize;
                    std::swap_ranges(top, top + rowSize, bottom);
                }
            }

            frameRecorder_.submit(std::move(frame));
        });

    frameRecorder_.start(makeCaptureOutputPath(), settings_);
    resetSessionStats();
}

void CaptureController::stop() {
    frameRecorder_.stop();
    captureFps_ = 0.0f;
    blinkElapsed_ = 0.0;
    toggleShortcutHeld_ = false;
    BgfxContext::instance().callback().removeScreenShotCallback("capture");
}

void CaptureController::toggle() {
    if (frameRecorder_.isRecording()) {
        stop();
    }
    else {
        start();
    }
}

void CaptureController::onFrameRendered() {
    if (!frameRecorder_.isRecording()) {
        return;
    }

    const double frameInterval = 1.0 / static_cast<double>(activeSessionFps_);
    if (captureSubmitAccum_ < frameInterval) {
        return;
    }
    captureSubmitAccum_ -= frameInterval;

    bgfx::requestScreenShot(BGFX_INVALID_HANDLE, "capture"); // Вызывает кэллбэк с frameRecorder_.submit
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
