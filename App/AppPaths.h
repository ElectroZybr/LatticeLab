#pragma once

#include <string_view>

namespace AppPaths {
    inline constexpr std::string_view kUserDirectory = "User";
    inline constexpr std::string_view kUserSettingsPath = "User/user_settings.cfg";
    inline constexpr std::string_view kImguiIniPath = "User/imgui.ini";
    inline constexpr std::string_view kDefaultCaptureDirectory = "User/captures";
    inline constexpr std::string_view kBuiltInScenesDirectory = "Mods/Base/scenes";
    inline constexpr std::string_view kUserScenesDirectory = "User/scenes";
}
