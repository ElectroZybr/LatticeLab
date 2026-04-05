#include "Application.h"
#include "AppActions.h"
#include "AppSignals.h"
#include "UserSettings.h"
#include "CreateWindow.h"
#include "capture/CaptureController.h"
#include "capture/FrameRecorder.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>

#include "App/interaction/ToolsManager.h"
#include "Engine/Simulation.h"
#include "Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/RendererCapture.h"
#include "Signals/Signals.h"

#include "Scenes.h"

constexpr int FPS = 60;
constexpr int LPS = 20;

int Application::run() {
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) {
        return EXIT_FAILURE;
    }
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    SimBox box(Vec3f(50, 50, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 50, AtomData::Type::Z, false);

    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView, simulation.box());
    renderer->setAtomStorage(&simulation.atoms());
    renderer->setBondStorage(&simulation.bonds());
    renderer->drawBonds = true;
    renderer->speedColorMode = IRenderer::SpeedColorMode::GradientClassic;

    // загрузка пользовательских настроек
    CaptureController captureController;
    const UserSettings userSettings = UserSettingsIO::load();
    captureController.setSettings(userSettings.captureSettings);
    captureController.setOutputDirectory(userSettings.captureOutputDirectory);

    AppActions::init(simulation, renderer, window, gameView);
    Interface::init(window, simulation, renderer, captureController);
    EventManager::init(&window, &gameView, renderer, &simulation.box(), &simulation.atoms());
    ToolsManager::init(&window, &gameView, &box.grid, &box, renderer, &simulation.atoms(),
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
    bool captureToggleHeld = false;
    Signals::ScopedConnection captureToggleConnection(
        AppSignals::UI::ToggleCapture.connect([&]() {
            captureController.toggle(window);
        })
    );
    Signals::ScopedConnection captureDirectoryConnection(
        AppSignals::UI::SetCaptureDirectory.connect([&](std::string_view path) {
            captureController.setOutputDirectory(std::filesystem::path(path));
        })
    );

    constexpr double renderInterval = 1.0 / FPS;
    constexpr double logInterval = 1.0 / LPS;

    while (window.isOpen()) {
        Profiler::instance().beginFrame();
        const float deltaTime = clock.restart().asSeconds();
        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        renderer->camera.update(window);
        EventManager::poll();
        EventManager::frame(deltaTime);
        captureController.update(deltaTime);
        Interface::captureRecording = captureController.isRecording();
        Interface::captureFrameCount = captureController.savedFrameCount();
        Interface::captureFps = captureController.captureFps();
        Interface::captureBlinkElapsed = captureController.blinkElapsed();

        const bool captureKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8);
        if (captureKeyPressed && !captureToggleHeld) {
            AppSignals::UI::ToggleCapture.emit();
        }
        captureToggleHeld = captureKeyPressed;

        // обновление физики
        const double physicsInterval = 1.0 / Interface::getSimulationSpeed();
        if (physicsAccum >= physicsInterval) {
            if (!Interface::getPause()) {
                simulation.update();
                Profiler::instance().addCount("Simulation::steps");
                physicsAccum = 0.0;
            } else {
                physicsAccum = 0.0;
            }
        }

        // отрисовка кадра
        if (renderAccum >= renderInterval) {
            PROFILE_SCOPE("Application::RenderFrame");
            renderAccum -= renderInterval;
            Interface::Update();
            refreshAtomDebugViews(debugViews, simulation);
            renderer->drawShot(simulation.atoms(), simulation.box());
            ToolsManager::pickingSystem->getOverlay().draw(window);
            ImGui::SFML::Render(window);

            captureController.onFrameRendered(window);

            window.display();
        }

        Profiler::instance().endFrame();

        // обновление логов и данных счетчиков
        if (logAccum >= logInterval) {
            logAccum -= logInterval;
            Profiler::instance().updateRates(logInterval);
            refreshSimulationDebugViews(debugViews, simulation);
        }
    }

    captureController.stop(window);
    UserSettingsIO::save(UserSettings{
        .captureOutputDirectory = captureController.outputDirectory(),
        .captureSettings = captureController.settings(),
    });
    Interface::shutdown();
    return 0;
}
