#pragma once

#include <memory>

#include "Scenes.h"
#include "Engine/Simulation.h"
#include "App/interaction/Tools.h"
#include "GUI/interface/interface.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

#include "AppSignals.h"

namespace sf {
    class RenderWindow;
    class View;
}

namespace AppEvents {
    class Handler : public Signals::Trackable {
        void trackIOPanel(Simulation& simulation) {
            track(AppSignals::UI::ResizeBox.connect([&](const Vec3f& newSize) {
                simulation.sim_box.setSizeBox(newSize);
            }));
            track(AppSignals::UI::ClearSimulation.connect([&]() {
                simulation.clear();
                Tools::resetInteractionState();
            }));
            track(AppSignals::UI::CreateGas.connect([&]() {
                simulation.clear();
                Tools::resetInteractionState();
                Scenes::randomGas(simulation,
                                  Interface::ioPanel.gasAtomCount(),
                                  Interface::ioPanel.gasAtomType(),
                                  Interface::ioPanel.gasIs3D(),
                                  6.0,
                                  6.0,
                                  Interface::ioPanel.gasDensity());
            }));
            track(AppSignals::UI::CreateCrystal.connect([&]() {
                simulation.clear();
                Tools::resetInteractionState();
                Scenes::crystal(simulation, Interface::ioPanel.sceneAxisCount(), Interface::ioPanel.atomType(), Interface::ioPanel.sceneIs3D());
            }));
        }

        void trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
            track(AppSignals::UI::SetRender.connect([&](RendererType type) {
                std::unique_ptr<IRenderer> newRenderer;
                switch (type) {
                case RendererType::Renderer2D:
                    newRenderer = std::make_unique<Renderer2D>(window, gameView, simulation.sim_box);
                    break;
                case RendererType::Renderer3D:
                    newRenderer = std::make_unique<Renderer3D>(window, gameView, simulation.sim_box);
                    break;
                }

                if (newRenderer) {
                    newRenderer->drawGrid = renderer->drawGrid;
                    newRenderer->drawBonds = renderer->drawBonds;
                    newRenderer->speedColorMode = renderer->speedColorMode;
                    newRenderer->speedGradientMax = renderer->speedGradientMax;
                    newRenderer->setAtomStorage(&simulation.atomStorage);
                    renderer = std::move(newRenderer);
                }
            }));
        }
    public:
        Handler(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
            trackIOPanel(simulation);
            trackToolsPanel(simulation, renderer, window, gameView);
        }
    };

    inline Handler* instance = nullptr;

    inline void init(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
        instance = new Handler(simulation, renderer, window, gameView);
    }
}

inline void processFileDialog(Simulation& simulation) {
    if (auto result = Interface::fileDialog.popResult()) {
        switch (result->command) {
            case FileDialogCommand::Save:
                simulation.save(result->path);
                break;
            case FileDialogCommand::Load:
                simulation.load(result->path);
                Tools::resetInteractionState();
                break;
        }
    }
}

inline void processToolsPanel(std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window,
                              sf::View& gameView) {
    if (auto result = Interface::toolsPanel.popResult()) {
        switch (result.value()) {
            case ToolsCommand::SetCameraOrbit:
                renderer->camera.setMode(Camera::Mode::Orbit);
                break;
            case ToolsCommand::SetCameraFree:
                renderer->camera.setMode(Camera::Mode::Free);
                break;
        }
    }
}

inline void processSettingsPanel(sf::RenderWindow& window) {
    if (auto result = Interface::settingsPanel.popResult()) {
        switch (result.value()) {
            case SettingsCommand::ExitApplication:
                window.close();
                break;
        }
    }
}
