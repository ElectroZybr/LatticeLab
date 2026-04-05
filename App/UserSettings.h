#pragma once

#include <filesystem>

#include "App/capture/FrameRecorder.h"

struct UserSettings {
    std::filesystem::path captureOutputDirectory = "captures";
    CaptureSettings captureSettings{};
};

class UserSettingsIO {
public:
    static UserSettings load(const std::filesystem::path& path = defaultPath());
    static void save(const UserSettings& settings, const std::filesystem::path& path = defaultPath());

    static std::filesystem::path defaultPath();
};
