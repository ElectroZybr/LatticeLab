#pragma once

#include <memory>

#include "Signals/Signals.h"

class Simulation;
class IRenderer;
namespace sf {
    class RenderWindow;
    class View;
    class Window;
}

namespace AppActions {
    class Handler : public Signals::Trackable {
    public:
        Handler(sf::RenderWindow& window, sf::View& sceneView, Simulation& simulation, std::unique_ptr<IRenderer>& renderer);

    private:
        void trackIOPanel(sf::RenderWindow& window, Simulation& simulation, std::unique_ptr<IRenderer>& renderer);
        void trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& sceneView);
        void trackSettingsPanel(sf::Window& window);
        void trackKeyboard(Simulation& simulation);
        void trackSimControlPanel(Simulation& simulation);
    };
}
