#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <iostream>
#endif

#include "AppVersion.h"

#include <SFML/Graphics.hpp>

inline sf::RenderWindow createWindow() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
#ifdef __APPLE__
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Attribute::Core;
#endif

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "LatticeLab " LATTICELAB_VERSION_STRING, sf::State::Fullscreen, settings);
#ifdef __APPLE__
    const sf::ContextSettings actualSettings = window.getSettings();
    const bool hasModernContext = actualSettings.majorVersion > 4 ||
                                  (actualSettings.majorVersion == 4 && actualSettings.minorVersion >= 1) ||
                                  (actualSettings.majorVersion == 3 && actualSettings.minorVersion >= 2);
    const bool isCoreContext = (actualSettings.attributeFlags & sf::ContextSettings::Attribute::Core) != 0;
    if (!hasModernContext || !isCoreContext) {
        std::cerr << "Failed to create modern OpenGL core context on macOS. Actual context: " << actualSettings.majorVersion << "."
                  << actualSettings.minorVersion << std::endl;
        window.close();
        return window;
    }
#endif

#ifdef _WIN32
    if (const auto hwnd = static_cast<HWND>(window.getNativeHandle())) {
        HINSTANCE instance = GetModuleHandleW(nullptr);
        if (HICON bigIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR))) {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
        }
        if (HICON smallIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR))) {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        }
    }
#else
    sf::Image icon;
    if (icon.loadFromFile("assets/icon.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }
#endif

    return window;
}
