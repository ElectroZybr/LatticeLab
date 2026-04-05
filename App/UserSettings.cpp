#include "UserSettings.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace {
const char* presetToString(CaptureSettings::Preset preset) {
    switch (preset) {
        case CaptureSettings::Preset::Ultrafast: return "ultrafast";
        case CaptureSettings::Preset::Veryfast:  return "veryfast";
        case CaptureSettings::Preset::Faster:    return "faster";
        case CaptureSettings::Preset::Fast:      return "fast";
        case CaptureSettings::Preset::Medium:    return "medium";
    }
    return "veryfast";
}

CaptureSettings::Preset presetFromString(const std::string& value) {
    if (value == "ultrafast") return CaptureSettings::Preset::Ultrafast;
    if (value == "veryfast")  return CaptureSettings::Preset::Veryfast;
    if (value == "faster")    return CaptureSettings::Preset::Faster;
    if (value == "fast")      return CaptureSettings::Preset::Fast;
    if (value == "medium")    return CaptureSettings::Preset::Medium;
    return CaptureSettings::Preset::Veryfast;
}

const char* pixelFormatToString(CaptureSettings::PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case CaptureSettings::PixelFormat::Yuv420p: return "yuv420p";
        case CaptureSettings::PixelFormat::Yuv444p: return "yuv444p";
    }
    return "yuv444p";
}

CaptureSettings::PixelFormat pixelFormatFromString(const std::string& value) {
    if (value == "yuv420p") return CaptureSettings::PixelFormat::Yuv420p;
    return CaptureSettings::PixelFormat::Yuv444p;
}
}

std::filesystem::path UserSettingsIO::defaultPath() {
    return "user_settings.cfg";
}

UserSettings UserSettingsIO::load(const std::filesystem::path& path) {
    UserSettings settings;

    std::ifstream file(path);
    if (!file.is_open()) {
        return settings;
    }

    std::string tag;
    while (file >> tag) {
        if (tag == "capture_output_dir") {
            std::string value;
            if (file >> std::ws && std::getline(file, value) && !value.empty()) {
                settings.captureOutputDirectory = value;
            }
        } else if (tag == "capture_fps") {
            file >> settings.captureSettings.fps;
        } else if (tag == "capture_crf") {
            file >> settings.captureSettings.crf;
        } else if (tag == "capture_preset") {
            std::string value;
            file >> value;
            settings.captureSettings.preset = presetFromString(value);
        } else if (tag == "capture_pixel_format") {
            std::string value;
            file >> value;
            settings.captureSettings.pixelFormat = pixelFormatFromString(value);
        } else {
            std::string ignoredLine;
            std::getline(file, ignoredLine);
        }
    }

    if (settings.captureSettings.fps < 1) {
        settings.captureSettings.fps = 30;
    }
    settings.captureSettings.crf = std::clamp(settings.captureSettings.crf, 0, 51);
    if (settings.captureOutputDirectory.empty()) {
        settings.captureOutputDirectory = "captures";
    }

    return settings;
}

void UserSettingsIO::save(const UserSettings& settings, const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }

    file << "capture_output_dir " << settings.captureOutputDirectory.string() << "\n";
    file << "capture_fps " << settings.captureSettings.fps << "\n";
    file << "capture_crf " << settings.captureSettings.crf << "\n";
    file << "capture_preset " << presetToString(settings.captureSettings.preset) << "\n";
    file << "capture_pixel_format " << pixelFormatToString(settings.captureSettings.pixelFormat) << "\n";
}
