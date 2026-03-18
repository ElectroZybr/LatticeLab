#include "FontManager.h"

#define ICON_MIN_FA 0xf000
#define ICON_MAX_FA 0xf897

bool FontManager::load() {
    auto* fonts = ImGui::GetIO().Fonts;

    // Основной шрифт
    main = fonts->AddFontFromFileTTF(
        "GUI/fonts/Rubik-VariableFont_wght.ttf", 50.0f);
    if (!main) return false;

    // Иконки мержим в основной
    ImFontConfig iconConfig;
    iconConfig.MergeMode        = true;
    iconConfig.GlyphMinAdvanceX = 40.0f;
    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    fonts->AddFontFromFileTTF(
        "GUI/fonts/Font Awesome 5 Free-Solid-900.otf",
        40.0f, &iconConfig, iconRanges);

    // Диалоговый шрифт с кириллицей
    static const ImWchar ruRanges[] = {
        0x0020, 0x00FF,
        0x0400, 0x04FF,
        0,
    };
    dialog = fonts->AddFontFromFileTTF(
        "GUI/fonts/Rubik-VariableFont_wght.ttf",
        20.0f, nullptr, ruRanges);
    if (!dialog) return false;

    return true;
}