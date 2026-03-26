#include "Application.h"
#include "CreateDebugPanels.h"
#include "UpdateDebugData.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include "imgui-SFML.h"

#include "Engine/Simulation.h"
#include "Engine/Tools.h"
#include "Engine/utils/Timer.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"
#include "memory_monitor.h"

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

namespace Scenes {
void crystal(Simulation& sim, int n, AtomData::Type type, bool is3d,
             double padding = 3.0, double margin = 15.0) {
    const double side = n * padding + padding + 2.0 * margin;
    const double half = side / 2.0;

    sim.setSizeBox(
        Vec3f(-half, -half, is3d ? -half : sim.sim_box.start.z),
        Vec3f(half, half, is3d ? half : sim.sim_box.end.z)
    );

    const Vec3f vecMargin(margin, margin, is3d ? margin : 0.0);
    const int zMax = is3d ? n : 1;

    for (int x = 1; x <= n; ++x) {
        for (int y = 1; y <= n; ++y) {
            for (int z = 1; z <= zMax; ++z) {
                sim.createAtom(Vec3f(x, y, z) * padding + vecMargin,
                               Vec3f::Random() * 0.5, type);
            }
        }
    }
}
} // namespace Scenes

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


struct RateCounter {
    Timer timer;
    double accumulatedMs = 0.0;
    int stepsThisTick = 0;
    float stepsPerSecond = 0.0f;
    bool hasRateSample = false;

    void startStep() { timer.start(); }

    void finishStep() {
        timer.stop();
        accumulatedMs += timer.elapsedMilliseconds();
        ++stepsThisTick;
    }

    float avgMs() const {
        return stepsThisTick > 0 ? static_cast<float>(accumulatedMs / stepsThisTick) : 0.0f;
    }

    void flush(double elapsedSeconds) {
        if (elapsedSeconds > 0.0) {
            const float instantRate = static_cast<float>(stepsThisTick / elapsedSeconds);
            const float alpha = static_cast<float>(1.0 - std::exp(-elapsedSeconds / 0.5));
            if (!hasRateSample) {
                stepsPerSecond = instantRate;
                hasRateSample = true;
            } else {
                stepsPerSecond += alpha * (instantRate - stepsPerSecond);
            }
        }
        accumulatedMs = 0.0;
        stepsThisTick = 0;
    }
};

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
        if (result.value() == ToolsCommand::ClearSimulation) {
            simulation.clear();
            Tools::resetInteractionState();
            return;
        }

        std::unique_ptr<IRenderer> newRenderer;
        switch (result.value()) {
            case ToolsCommand::ToggleRenderer2D:
                newRenderer = std::make_unique<Renderer2D>(window, gameView);
                break;
            case ToolsCommand::ToggleRenderer3D:
                newRenderer = std::make_unique<Renderer3D>(window, gameView);
                break;
            case ToolsCommand::ClearSimulation:
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

    SimBox box(Vec3f(-25, -25, 0), Vec3f(25, 25, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 500, AtomData::Type::Z, false);

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

            renderer->camera.update(window);

            renderCounter.startStep();
            renderer->drawShot(simulation.atomStorage, simulation.sim_box);
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


