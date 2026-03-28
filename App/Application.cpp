#include "Application.h"
#include "debug/CreateDebugPanels.h"
#include "debug/UpdateDebugData.h"
#include <cmath>
#include <cstdlib>
#include <string_view>
#ifdef __APPLE__
#include <iostream>
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include "imgui-SFML.h"

#include "Engine/Simulation.h"
#include "Engine/tools/Tools.h"
#include "Engine/utils/RateCounter.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

#include "Scenes.h"

namespace {

constexpr int FPS = 60;
constexpr int LPS = 20;
constexpr double Dt = 0.01;

std::string_view schemeName(Integrator::Scheme scheme) {
    switch (scheme) {
        case Integrator::Scheme::Verlet:   return "Velocity Verlet";
        case Integrator::Scheme::KDK:      return "KDK (Kick-Drift-Kick)";
        case Integrator::Scheme::RK4:      return "Runge-Kutta 4";
        case Integrator::Scheme::Langevin: return "Langevin";
    }
    return "Unknown";
}

sf::RenderWindow createWindow() {
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

void processFileDialog(Simulation& simulation) {
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

void processToolsPanel(std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window,
                       sf::View& gameView, Simulation& simulation) {
    if (auto result = Interface::toolsPanel.popResult()) {
        std::unique_ptr<IRenderer> newRenderer;
        switch (result.value()) {
            case ToolsCommand::ToggleRenderer2D:
                newRenderer = std::make_unique<Renderer2D>(window, gameView);
                break;
            case ToolsCommand::ToggleRenderer3D:
                newRenderer = std::make_unique<Renderer3D>(window, gameView);
                break;
            case ToolsCommand::ClearSimulation:
                simulation.clear();
                Tools::resetInteractionState();
                break;
            case ToolsCommand::SetCameraOrbit:
                renderer->camera.setMode(Camera::Mode::Orbit);
                break;
                case ToolsCommand::SetCameraFree:
                renderer->camera.setMode(Camera::Mode::Free);
                break;
        }

        if (newRenderer) {
            newRenderer->drawGrid = renderer->drawGrid;
            newRenderer->drawBonds = renderer->drawBonds;
            newRenderer->speedGradient = renderer->speedGradient;
            newRenderer->speedGradientTurbo = renderer->speedGradientTurbo;
            newRenderer->speedGradientMax = renderer->speedGradientMax;
            newRenderer->setAtomStorage(&simulation.atomStorage);
            renderer = std::move(newRenderer);
        }
    }
}

} // namespace

int Application::run() {
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) {
        return EXIT_FAILURE;
    }
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    SimBox box(Vec3f(-25, -25, -3), Vec3f(25, 25, 3));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 30, AtomData::Type::Z, false);

    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView);
    renderer->setAtomStorage(&simulation.atomStorage);
    renderer->drawBonds = true;
    renderer->speedGradient = true;

    Interface::init(window, simulation, renderer);
    EventManager::init(&window, &gameView, renderer, &simulation.sim_box, &simulation.atomStorage);
    Tools::init(&window, &gameView, &box.grid, &box, renderer, &simulation.atomStorage,
                [&](Vec3f coords, Vec3f speed, AtomData::Type type, bool fixed) {
                    return simulation.createAtom(coords, speed, type, fixed);
                },
                [&](std::size_t atomIndex) {
                    return simulation.removeAtom(atomIndex);
                });
    Interface::pause = true;

    const DebugViews debugViews = createDebugViews(Interface::debugPanel);

    sf::Clock clock;
    RateCounter physicsCounter;
    RateCounter renderCounter;

    double renderAccum = 0.0;
    double physicsAccum = 0.0;
    double logAccum = 0.0;

    constexpr double renderInterval = 1.0 / FPS;
    constexpr double logInterval = 1.0 / LPS;

    while (window.isOpen()) {
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
                physicsCounter.startStep();
                simulation.update(Dt);
                physicsCounter.finishStep();
                physicsAccum = 0.0;
            }
        } else {
            physicsAccum = 0.0;
        }

        // один шаг симуляции
        if (auto cmd = Keyboard::popResult(); cmd == KeyboardCommand::StepPhysics) {
            physicsCounter.startStep();
            simulation.update(Dt);
            physicsCounter.finishStep();
        }

        if (renderAccum >= renderInterval) {
            renderAccum -= renderInterval;

            Interface::Update();

            updateAtomSelectionDebug(debugViews, simulation);

            processFileDialog(simulation);
            processToolsPanel(renderer, window, gameView, simulation);

            renderCounter.startStep();
            renderer->drawShot(simulation.atomStorage, simulation.sim_box);
            Tools::pickingSystem->getOverlay().draw(window);
            ImGui::SFML::Render(window);
            window.display();
            renderCounter.finishStep();
        }

        // обновение логов и данных счетчиков
        if (logAccum >= logInterval) {
            logAccum -= logInterval;
            updateSimulationDebug(debugViews, simulation, renderCounter.avgMs(), physicsCounter.avgMs(),
                                  physicsCounter.stepsPerSecond, schemeName(simulation.getIntegrator()));

            physicsCounter.flush(logInterval);
            renderCounter.flush(logInterval);
        }
    }

    Interface::shutdown();
    return 0;
}


