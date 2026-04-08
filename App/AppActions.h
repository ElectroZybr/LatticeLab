#pragma once

#include <memory>

#include "App/AppSignals.h"
#include "App/AppStateIO.h"
#include "App/Scenes.h"
#include "App/interaction/ToolsManager.h"
#include "Engine/Simulation.h"
#include "Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

namespace sf {
    class RenderWindow;
    class View;
}

namespace AppActions {
    inline void shiftAtoms(AtomStorage& atomStorage, Vec3f delta) {
        float* x = atomStorage.xData();
        float* y = atomStorage.yData();
        float* z = atomStorage.zData();
        const size_t atomCount = atomStorage.size();

        for (size_t i = 0; i < atomCount; ++i) {
            x[i] += delta.x;
            y[i] += delta.y;
            z[i] += delta.z;
        }
    }

    inline void applyResizeBox(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, const Vec3f& newSize) {
        const Vec3f oldSize = simulation.box().size;
        const Vec3f delta = (newSize - oldSize) * 0.5f;

        shiftAtoms(simulation.atoms(), delta);
        renderer->camera.move({delta.x, delta.y});
        renderer->camera.move3D(delta);
        simulation.setSizeBox(newSize);
    }

    class Handler : public Signals::Trackable {
        void trackIOPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, Interface& ui) {
            track(AppSignals::UI::SaveSimulation.connect([&](std::string_view path) { AppStateIO::save(simulation, *renderer, path); }));
            track(AppSignals::UI::LoadSimulation.connect([&](std::string_view path) {
                AppStateIO::load(simulation, *renderer, path);
                ToolsManager::resetInteractionState();
            }));
            track(AppSignals::UI::ResizeBox.connect([&](const Vec3f& newSize) { applyResizeBox(simulation, renderer, newSize); }));
            track(AppSignals::UI::ClearSimulation.connect([&]() {
                simulation.clear();
                ToolsManager::resetInteractionState();
            }));
            track(AppSignals::UI::CreateGas.connect([&]() {
                simulation.clear();
                ToolsManager::resetInteractionState();
                IOPanel& ioPanel = ui.ioPanel;
                Scenes::randomGas(simulation, ioPanel.gasAtomCount(), ioPanel.gasAtomType(), ioPanel.gasIs3D(), 6.0, 6.0, ioPanel.gasDensity());
            }));
            track(AppSignals::UI::CreateCrystal.connect([&]() {
                simulation.clear();
                ToolsManager::resetInteractionState();
                IOPanel& ioPanel = ui.ioPanel;
                Scenes::crystal(simulation, ioPanel.sceneAxisCount(), ioPanel.atomType(), ioPanel.sceneIs3D());
            }));
        }

        void trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView) {
            track(AppSignals::UI::SetRender.connect([&](RendererType type) {
                std::unique_ptr<IRenderer> newRenderer;
                switch (type) {
                case RendererType::Renderer2D:
                    newRenderer = std::make_unique<Renderer2D>(window, gameView, simulation.box());
                    break;
                case RendererType::Renderer3D:
                    newRenderer = std::make_unique<Renderer3D>(window, gameView, simulation.box());
                    break;
                }

                if (newRenderer) {
                    ToolsManager::resetInteractionState();
                    newRenderer->drawGrid = renderer->drawGrid;
                    newRenderer->drawBonds = renderer->drawBonds;
                    newRenderer->speedColorMode = renderer->speedColorMode;
                    newRenderer->speedGradientMax = renderer->speedGradientMax;
                    renderer = std::move(newRenderer);
                }
            }));

            track(AppSignals::UI::SetCameraMode.connect([&](Camera::Mode mode) { renderer->camera.setMode(mode); }));
        }

        void trackSettingsPanel(sf::Window& window) { track(AppSignals::UI::ExitApplication.connect(&sf::Window::close, &window)); }

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
        Handler(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView, Interface& ui) {
            trackIOPanel(simulation, renderer, ui);
            trackToolsPanel(simulation, renderer, window, gameView);
            trackSettingsPanel(window);
            trackSimControlPanel(simulation);
            trackKeyboard(simulation);
        }
    };

    inline Handler* instance = nullptr;

    inline void init(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window, sf::View& gameView, Interface& ui) {
        instance = new Handler(simulation, renderer, window, gameView, ui);
    }
}
