#include "FontManager.h"
#include <algorithm>
#include <cmath>

#define ICON_MIN_FA 0xf000
#define ICON_MAX_FA 0xf897

bool FontManager::load(float uiScale) {
    auto* fonts = ImGui::GetIO().Fonts;
    uiScale = std::clamp(uiScale, 0.75f, 3.0f);
    fonts->Clear();

    const auto scaledSize = [uiScale](float basePx) {
        return std::max(12.0f, std::round(basePx * uiScale));
    };

    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    static const ImWchar ruRanges[] = {
        0x0020, 0x00FF,
        0x0400, 0x04FF,
        0,
    };

    ImFontConfig mainConfig;
    mainConfig.OversampleH = 4;
    mainConfig.OversampleV = 4;
    main = fonts->AddFontFromFileTTF(
        "assets/fonts/Rubik-VariableFont_wght.ttf", scaledSize(37.5f), &mainConfig);
    if (!main) return false;

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    iconConfig.GlyphMinAdvanceX = scaledSize(30.0f);
    if (!fonts->AddFontFromFileTTF(
        "assets/fonts/Font Awesome 5 Free-Solid-900.otf",
        scaledSize(30.0f), &iconConfig, iconRanges)) {
        return false;
    }

    ImFontConfig sideIconConfig;
    sideIconConfig.PixelSnapH = true;
    sideIconConfig.OversampleH = 4;
    sideIconConfig.OversampleV = 4;
    icons = fonts->AddFontFromFileTTF(
        "assets/fonts/Font Awesome 5 Free-Solid-900.otf",
        scaledSize(22.5f), &sideIconConfig, iconRanges);
    if (!icons) return false;

    ImFontConfig dialogConfig;
    dialogConfig.OversampleH = 4;
    dialogConfig.OversampleV = 4;
    dialog = fonts->AddFontFromFileTTF(
        "assets/fonts/Rubik-VariableFont_wght.ttf",
        scaledSize(18.0f), &dialogConfig, ruRanges);
    if (!dialog) return false;

    fonts->Build();
    return true;
}
