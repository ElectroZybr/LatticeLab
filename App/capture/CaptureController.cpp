#include "CaptureController.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

CaptureSettings CaptureController::settings() const noexcept {
    return settings_;
}

void CaptureController::setSettings(const CaptureSettings& settings) noexcept {
    settings_ = settings;
}

std::filesystem::path CaptureController::outputDirectory() const {
    return outputDirectory_;
}

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

void CaptureController::start() {
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
}

void CaptureController::toggle(sf::RenderWindow& window) {
    if (frameRecorder_.isRecording()) {
        stop(window);
    } else {
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

bool CaptureController::isRecording() const {
    return frameRecorder_.isRecording();
}

uint64_t CaptureController::savedFrameCount() const {
    return frameRecorder_.isRecording() ? frameRecorder_.savedFrameCount() : 0;
}

float CaptureController::captureFps() const noexcept {
    return frameRecorder_.isRecording() ? captureFps_ : 0.0f;
}

double CaptureController::blinkElapsed() const noexcept {
    return frameRecorder_.isRecording() ? blinkElapsed_ : 0.0;
}

std::filesystem::path CaptureController::makeCaptureOutputPath() const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream datePrefix;
    datePrefix << std::put_time(&localTime, "%Y-%m-%d");

    const std::filesystem::path capturesDir = outputDirectory_.empty()
        ? std::filesystem::path("captures")
        : outputDirectory_;
    std::filesystem::create_directories(capturesDir);

    const std::string prefix = datePrefix.str() + "_";
    int nextIndex = 1;

    for (const auto& entry : std::filesystem::directory_iterator(capturesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path path = entry.path();
        if (path.extension() != ".mp4") {
            continue;
        }

        const std::string stem = path.stem().string();
        if (!stem.starts_with(prefix)) {
            continue;
        }

        const std::string suffix = stem.substr(prefix.size());
        try {
            nextIndex = std::max(nextIndex, std::stoi(suffix) + 1);
        } catch (...) {
        }
    }

    return capturesDir / (prefix + std::to_string(nextIndex) + ".mp4");
}

void CaptureController::resetSessionStats() {
    captureRateAccum_ = 0.0;
    captureSubmitAccum_ = 0.0;
    blinkElapsed_ = 0.0;
    lastCaptureFrameCountSample_ = 0;
    captureFps_ = 0.0f;
}
