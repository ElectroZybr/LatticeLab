#include "Application.h"
#include "AppActions.h"
#include "CreateWindow.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"
#include <cmath>
#include <cstdlib>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>

#include "Engine/Simulation.h"
#include "Engine/metrics/Profiler.h"
#include "App/interaction/Tools.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/2d/Renderer2D.h"

#include "Scenes.h"

constexpr int FPS = 60;
constexpr int LPS = 20;

int Application::run() {
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) {
        return EXIT_FAILURE;
    }
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    SimBox box(Vec3f(50, 50, 50));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 25, AtomData::Type::Z, false);

    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView, simulation.sim_box);
    renderer->setAtomStorage(&simulation.atomStorage);
    renderer->drawBonds = true;
    renderer->speedColorMode = IRenderer::SpeedColorMode::GradientClassic;

    AppEvents::init(simulation);
    Interface::init(window, simulation, renderer);
    EventManager::init(&window, &gameView, renderer, &simulation.sim_box, &simulation.atomStorage);
    Tools::init(&window, &gameView, &box.grid, &box, renderer, &simulation.atomStorage,
                [&](Vec3f coords, Vec3f speed, AtomData::Type type, bool fixed) {
                    return simulation.createAtom(coords, speed, type, fixed);
                },
                [&](size_t atomIndex) {
                    return simulation.removeAtom(atomIndex);
                });
    Interface::pause = true;

    const DebugViews debugViews = createDebugViews(Interface::debugPanel);

    sf::Clock clock;
    double renderAccum = 0.0;
    double physicsAccum = 0.0;
    double logAccum = 0.0;

    constexpr double renderInterval = 1.0 / FPS;
    constexpr double logInterval = 1.0 / LPS;

    while (window.isOpen()) {
        Profiler::instance().beginFrame();
        float deltaTime = clock.restart().asSeconds();
        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        renderer->camera.update(window);
        EventManager::poll();
        EventManager::frame(deltaTime);

        if (!Interface::getPause()) {
            const double physicsInterval = 1.0 / Interface::getSimulationSpeed();
            if (physicsAccum >= physicsInterval) {
                simulation.update();
                Profiler::instance().addCount("Simulation::steps");
                physicsAccum = 0.0;
            }
        } else {
            physicsAccum = 0.0;
        }

        // один шаг симуляции
        if (auto cmd = Keyboard::popResult(); cmd == KeyboardCommand::StepPhysics) {
            simulation.update();
            Profiler::instance().addCount("Simulation::steps");
        }
        if (Interface::popStepRequested()) {
            simulation.update();
            Profiler::instance().addCount("Simulation::steps");
        }

        if (renderAccum >= renderInterval) {
            renderAccum -= renderInterval;

            PROFILE_SCOPE("Application::RenderFrame");
            
            Interface::Update();

            refreshAtomDebugViews(debugViews, simulation);

            processFileDialog(simulation);
            processToolsPanel(renderer, window, gameView, simulation);
            processIOPanel(simulation, renderer);
            processSettingsPanel(window);

            renderer->drawShot(simulation.atomStorage, simulation.sim_box);
            Tools::pickingSystem->getOverlay().draw(window);
            ImGui::SFML::Render(window);
            window.display();
        }

        Profiler::instance().endFrame();

        // обновение логов и данных счетчиков
        if (logAccum >= logInterval) {
            logAccum -= logInterval;
            Profiler::instance().updateRates(logInterval);
            refreshSimulationDebugViews(debugViews, simulation);
        }
    }

    Interface::shutdown();
    return 0;
}
