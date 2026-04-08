#include "Application.h"
#include "AppActions.h"
#include "AppSignals.h"
#include "CreateWindow.h"
#include "Scenes.h"
#include "UserSettings.h"
#include "capture/CaptureActions.h"
#include "capture/CaptureController.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"

#include <cmath>
#include <cstdlib>

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
    // создание окна
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) { return EXIT_FAILURE; }
    sf::View& sceneView = const_cast<sf::View&>(window.getView());    
    
    // инициализация систем
    SimBox box(Vec3f(50, 50, 6));
    Simulation simulation(box);
    CaptureController captureController;
    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, sceneView, simulation.box());
    Interface appInterface(window, simulation, renderer, captureController);
    AppActions::Handler appActions(window, sceneView, simulation, renderer);
    CaptureActions::Handler captureActions(window, captureController);
    if (appInterface.init() != EXIT_SUCCESS) { return EXIT_FAILURE; }
    EventManager::init(window, sceneView, simulation, renderer, appInterface);
    ToolsManager::init(window, sceneView, simulation, renderer, appInterface);
    const DebugViews debugViews = createDebugViews(appInterface.debugPanel);
    
    // загрузка пользовательских настроек
    const UserSettings userSettings = UserSettingsIO::load();
    captureController.setSettings(userSettings.captureSettings);
    captureController.setOutputDirectory(userSettings.captureOutputDirectory);

    // начальная настройка симуляции и рендерера
    renderer->drawBonds = true;
    renderer->speedColorMode = IRenderer::SpeedColorMode::AtomColor;
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    appInterface.state().pause = true;

    // создание сцены
    // Scenes::crystal(simulation, 50, AtomData::Type::Z, false);
    // simulation.createAtom(Vec3f(24, 25, 3), Vec3f(1, 0, 0), AtomData::Type::Na);
    // simulation.createAtom(Vec3f(28, 25, 3), Vec3f(-1, 0, 0), AtomData::Type::Na);

    sf::Clock clock;
    double renderAccum = 0.0;
    double physicsAccum = 0.0;
    double logAccum = 0.0;

    constexpr double renderInterval = 1.0 / FPS;
    constexpr double logInterval = 1.0 / LPS;

    while (window.isOpen()) {
        Profiler::instance().beginFrame();
        const float deltaTime = clock.restart().asSeconds();
        UiState& uiState = appInterface.state();
        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        renderer->camera.update(window);
        EventManager::poll();
        EventManager::frame(deltaTime);
        captureController.update(deltaTime);
        captureController.syncUiState(uiState);
        captureController.handleToggleShortcut(window);

        // обновление физики
        const double physicsInterval = 1.0 / uiState.simulationSpeed;
        if (physicsAccum >= physicsInterval) {
            if (!uiState.pause) {
                simulation.update();
            }
            physicsAccum = 0.0;
        }

        // отрисовка кадра
        if (renderAccum >= renderInterval) {
            PROFILE_SCOPE("Application::RenderFrame");
            renderAccum -= renderInterval;
            uiState.simStep = simulation.getSimStep();
            appInterface.update();
            refreshAtomDebugViews(debugViews, simulation);
            renderer->drawShot(simulation.atoms(), simulation.bonds(), simulation.box());
            ToolsManager::pickingSystem->getOverlay().draw(window);
            ImGui::SFML::Render(window);

            // захват кадра для видео
            captureController.onFrameRendered(window);

            window.display();
        }

        Profiler::instance().endFrame();

        // обновление логов и данных счетчиков
        if (logAccum >= logInterval) {
            logAccum -= logInterval;
            refreshSimulationDebugViews(debugViews, simulation);
        }
    }

    captureController.stop(window);
    UserSettingsIO::save(UserSettings{
        .captureOutputDirectory = captureController.outputDirectory(),
        .captureSettings = captureController.settings(),
    });
    appInterface.shutdown();
    return 0;
}
