#include "Application.h"
#include "AppActions.h"
#include "AppSignals.h"
#include "CreateWindow.h"
#include "Scenes.h"
#include "UserSettings.h"
#include "capture/CaptureController.h"
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
#include "Signals/Signals.h"

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
    // Scenes::crystal(simulation, 50, AtomData::Type::Z, false);
    // simulation.createAtom(Vec3f(24, 25, 3), Vec3f(1, 0, 0), AtomData::Type::Na);
    // simulation.createAtom(Vec3f(28, 25, 3), Vec3f(-1, 0, 0), AtomData::Type::Na);

    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView, simulation.box());
    renderer->drawBonds = true;
    renderer->speedColorMode = IRenderer::SpeedColorMode::AtomColor;

    // загрузка пользовательских настроек
    CaptureController captureController;
    const UserSettings userSettings = UserSettingsIO::load();
    captureController.setSettings(userSettings.captureSettings);
    captureController.setOutputDirectory(userSettings.captureOutputDirectory);
    Interface ui(window, simulation, renderer, captureController);
    AppActions::Handler appActions(window, gameView, simulation, renderer, ui);

    if (ui.init() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    EventManager::init(window, gameView, simulation, renderer, ui);
    ToolsManager::init(window, gameView, simulation, renderer, ui);
    ui.state().pause = true;

    const DebugViews debugViews = createDebugViews(ui.debugPanel);

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
        UiState& uiState = ui.state();
        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        renderer->camera.update(window);
        EventManager::poll();
        EventManager::frame(deltaTime);
        captureController.update(deltaTime);
        uiState.captureAvailable = captureController.isAvailable();
        uiState.captureRecording = captureController.isRecording();
        uiState.captureFrameCount = captureController.savedFrameCount();
        uiState.captureFps = captureController.captureFps();
        uiState.captureBlinkElapsed = captureController.blinkElapsed();

        const bool captureKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8);
        if (uiState.captureAvailable && captureKeyPressed && !captureToggleHeld) {
            AppSignals::UI::ToggleCapture.emit();
        }
        captureToggleHeld = captureKeyPressed;

        // обновление физики
        const double physicsInterval = 1.0 / uiState.simulationSpeed;
        if (physicsAccum >= physicsInterval) {
            if (!uiState.pause) {
                simulation.update();
                Profiler::instance().addCount("Simulation::steps");
                physicsAccum = 0.0;
            }
            else {
                physicsAccum = 0.0;
            }
        }

        // отрисовка кадра
        if (renderAccum >= renderInterval) {
            PROFILE_SCOPE("Application::RenderFrame");
            renderAccum -= renderInterval;
            ui.update();
            refreshAtomDebugViews(debugViews, simulation);
            renderer->drawShot(simulation.atoms(), simulation.bonds(), simulation.box());
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
    ui.shutdown();
    return 0;
}
