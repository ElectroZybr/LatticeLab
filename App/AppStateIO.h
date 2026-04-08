#pragma once

#include <string_view>

namespace sf {
    class RenderWindow;
}

class Simulation;
class IRenderer;

class AppStateIO {
public:
    static void save(const sf::RenderWindow& window, const Simulation& simulation, const IRenderer& renderer, std::string_view path);
    static void load(Simulation& simulation, IRenderer& renderer, std::string_view path);
};
