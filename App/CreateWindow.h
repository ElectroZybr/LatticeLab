#pragma once

#ifdef __APPLE__
#include <iostream>
#endif

#include <SFML/Graphics.hpp>

inline sf::RenderWindow createWindow() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
#ifdef __APPLE__
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Attribute::Core;
#endif

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator",
                            sf::State::Fullscreen, settings);
#ifdef __APPLE__
    const sf::ContextSettings actualSettings = window.getSettings();
    const bool hasModernContext = actualSettings.majorVersion > 4
        || (actualSettings.majorVersion == 4 && actualSettings.minorVersion >= 1)
        || (actualSettings.majorVersion == 3 && actualSettings.minorVersion >= 2);
    const bool isCoreContext = (actualSettings.attributeFlags & sf::ContextSettings::Attribute::Core) != 0;
    if (!hasModernContext || !isCoreContext) {
        std::cerr << "Failed to create modern OpenGL core context on macOS. Actual context: "
                  << actualSettings.majorVersion << "." << actualSettings.minorVersion << std::endl;
        window.close();
        return window;
    }
#endif

    sf::Image icon;
    if (icon.loadFromFile("assets/icon.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    return window;
}
