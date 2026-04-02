#pragma once

#include <memory>

#include "Scenes.h"
#include "Engine/Simulation.h"
#include "App/interaction/Tools.h"
#include "GUI/interface/interface.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"
#include "Engine/io/SimulationStateIO.h"
#include "Engine/metrics/Profiler.h"

#include "AppSignals.h"

namespace sf {
    class RenderWindow;
    class View;
}

namespace AppActions {
    class Handler : public Signals::Trackable {
        void trackIOPanel(Simulation& simulation) {
            track(AppSignals::UI::SaveSimulation.connect([&](std::string_view path) {
                SimulationStateIO::save(simulation, path);
            }));
            track(AppSignals::UI::LoadSimulation.connect([&](std::string_view path) {
                SimulationStateIO::load(simulation, path);
                Tools::resetInteractionState();
            }));
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

            track(AppSignals::UI::SetCameraMode.connect([&](Camera::Mode mode) {
                renderer->camera.setMode(mode);
            }));
        }

        void trackSettingsPanel(sf::Window& window) {
            track(AppSignals::UI::ExitApplication.connect(&sf::Window::close, &window));
        }

        void trackKeyboard(Simulation& simulation) {
            track(AppSignals::Keyboard::StepPhysics.connect([&]() {
                simulation.update();
                Profiler::instance().addCount("Simulation::steps");
            }));
        }

        void trackSimControlPanel(Simulation& simulation) {
            track(AppSignals::UI::StepPhysics.connect([&]() {
                simulation.update();
                Profiler::instance().addCount("Simulation::steps");
            }));
        }
    public:
        Handler(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
            trackIOPanel(simulation);
            trackToolsPanel(simulation, renderer, window, gameView);
            trackSettingsPanel(window);
            trackSimControlPanel(simulation);
            trackKeyboard(simulation);
        }
    };

    inline Handler* instance = nullptr;

    inline void init(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
        instance = new Handler(simulation, renderer, window, gameView);
    }
}
