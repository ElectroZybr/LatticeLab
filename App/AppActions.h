#pragma once

#include <memory>

#include "Signals/Signals.h"

class Simulation;
class IRenderer;
class Interface;

namespace sf {
    class RenderWindow;
    class View;
    class Window;
}

namespace AppActions {
    class Handler : public Signals::Trackable {
    public:
        Handler(sf::RenderWindow& window, sf::View& gameView, Simulation& simulation, std::unique_ptr<IRenderer>& renderer, Interface& ui);

    private:
        void trackIOPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, Interface& ui);
        void trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView);
        void trackSettingsPanel(sf::Window& window);
        void trackKeyboard(Simulation& simulation);
        void trackSimControlPanel(Simulation& simulation);
    };
}
