#include "Application.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "App/AppActions.h"
#include "App/CreateWindow.h"
#include "Lattice/Generators/Generators.h"
#include "App/UserSettings.h"
#include "App/viewport/SceneViewport.h"
#include "App/interaction/ToolsManager.h"
#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/WGPUContext.h"
#include "capture/CaptureActions.h"
#include "capture/CaptureController.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"

using Clock = std::chrono::high_resolution_clock;

constexpr int FPS = 60;
constexpr int LPS = 20;

namespace {
    uint32_t makeXYZStepInterval(float simulationStepsPerSecond, int captureFps) {
        const float sanitizedStepsPerSecond = std::max(simulationStepsPerSecond, 1.0f);
        const int sanitizedCaptureFps = std::max(captureFps, 1);
        return std::max(1u, static_cast<uint32_t>(std::lround(sanitizedStepsPerSecond / static_cast<float>(sanitizedCaptureFps))));
    }
}

int Application::run() {
    GLFWwindow* window = createWindow();
    if (!window) {
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    WGPUContext::instance().init(window, width, height);

    // инициализация систем
    Lattice::Simulation simulation;

    simulation.createWorld({120, 120, 120});

    CaptureController captureController;
    SceneViewport renderer(SceneViewport::RendererType::Renderer3D, captureController);
    renderer.syncScene(simulation);

    Interface appInterface(window, simulation, renderer.rendererHandle(), captureController);
    appInterface.toolsPanel.setRendererType(renderer.renderer().camera.getMode() == Camera::Mode::Mode2D ? RendererType::Renderer2D : RendererType::Renderer3D);

    AppActions::Handler appActions(window, captureController, simulation, renderer, appInterface.state());
    CaptureActions::Handler captureActions(captureController);
    if (appInterface.init() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    EventManager::init(window, renderer.rendererHandle(), appInterface);
    ToolsManager::init(window, simulation, renderer.rendererHandle(), appInterface);
    const DebugViews debugViews = createDebugViews(appInterface.debugPanel);

    // загрузка пользовательских настроек
    const UserSettings userSettings = UserSettingsIO::load();
    captureController.setSettings(userSettings.captureSettings);
    captureController.setOutputDirectory(userSettings.captureOutputDirectory);
    appInterface.setScenesDirectory(userSettings.scenesDirectory);
    renderer.renderer().getRenderData(0).drawGrid = userSettings.rendererDrawGrid;
    renderer.renderer().getRenderData(0).drawBonds = userSettings.rendererDrawBonds;
    renderer.renderer().getRenderData(0).drawBox = userSettings.rendererDrawBox;
    renderer.renderer().getRenderData(0).speedColorMode = userSettings.rendererSpeedColorMode;
    renderer.renderer().getRenderData(0).speedGradientMax = userSettings.rendererSpeedGradientMax;
    simulation.world().getIntegrator().setScheme(userSettings.simulationIntegrator);
    simulation.world().setBondFormationEnabled(userSettings.simulationBondFormationEnabled);
    simulation.world().setLJEnabled(userSettings.simulationLJEnabled);
    simulation.world().setCoulombEnabled(userSettings.simulationCoulombEnabled);
    appInterface.state().simulationSpeed = 100.0f;
    appInterface.state().pause = true;

    // создание сцены
    Generators::triangularBipyramidCrystal(simulation, 8, AtomData::Type::Z);
    Generators::AngularVelocity(simulation, Vec3f(0.0f, 0.25f, 0.0f));
    renderer.syncScene(simulation);

    // std::vector<Scenes::AtomTypeSpec> gasSpecs = {
    //     // {AtomData::Type::O, 0, 80.0f},    // 80% водорода
    //     {AtomData::Type::Na, 0, 50.0f},   // 10% натрия
    //     {AtomData::Type::Cl, 0, 50.0f}    // 10% хлора
    // };
    // Scenes::randomGasMixed(Lattice::simulation, 500, gasSpecs, false, 6.0, 6.0, 1.0f, 5.0f, 0);
    // Lattice::simulation.createAtom(Vec3f(24, 25, 3), Vec3f(1, 0, 0), AtomData::Type::Na);
    // Lattice::simulation.createAtom(Vec3f(28, 25, 3), Vec3f(-1, 0, 0), AtomData::Type::Na);

    auto startTime = Clock::now();
    double renderAccum = 0.0;
    double physicsAccum = 0.0;
    double logAccum = 0.0;

    constexpr double renderInterval = 1.0 / FPS;
    constexpr double logInterval = 1.0 / LPS;

    renderer.setScreenSize(width, height);
    renderer.resetView();
    UiState& uiState = appInterface.state();

    while (!glfwWindowShouldClose(window)) {
        Profiler::instance().beginFrame();

        auto currentTime = Clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - startTime).count();
        startTime = currentTime;

        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        EventManager::poll();
        EventManager::frame(deltaTime);
        captureController.update(deltaTime);
        simulation.setXYZRecordingStepInterval(makeXYZStepInterval(uiState.simulationSpeed, captureController.settings().fps));
        captureController.syncUiState(uiState);
        uiState.xyzRecording = simulation.isXYZRecording();
        uiState.xyzFps = simulation.xyzFPS();
        uiState.xyzFrameCount = simulation.xyzFrameCount();
        captureController.handleToggleShortcut();

        // обновление физики
        const double physicsInterval = 1.0 / uiState.simulationSpeed;
        if (physicsAccum >= physicsInterval) {
            if (!uiState.pause) {
                simulation.updateAll();
            }
            physicsAccum = 0.0;
        }

        // отрисовка кадра
        if (renderAccum >= renderInterval) {
            renderAccum -= renderInterval;
            renderer.renderFrame(simulation, appInterface, debugViews);
        }

        Profiler::instance().endFrame();

        // обновление логов и данных счетчиков
        if (logAccum >= logInterval) {
            logAccum -= logInterval;
            refreshSimulationDebugViews(debugViews, simulation);
        }
    }

    captureController.stop();
    UserSettingsIO::save(UserSettings{
        .captureOutputDirectory = captureController.outputDirectory(),
        .scenesDirectory = appInterface.scenesDirectory(),
        .captureSettings = captureController.settings(),
        .rendererDrawGrid = renderer.renderer().getRenderData(0).drawGrid,
        .rendererDrawBonds = renderer.renderer().getRenderData(0).drawBonds,
        .rendererDrawBox = renderer.renderer().getRenderData(0).drawBox,
        .rendererSpeedColorMode = renderer.renderer().getRenderData(0).speedColorMode,
        .rendererSpeedGradientMax = renderer.renderer().getRenderData(0).speedGradientMax,
        .simulationIntegrator = simulation.world().getIntegrator().getScheme(),
        .simulationBondFormationEnabled = simulation.world().isBondFormationEnabled(),
        .simulationLJEnabled = simulation.isLJEnabled(),
        .simulationCoulombEnabled = simulation.isCoulombEnabled(),
    });
    appInterface.shutdown();
    return 0;
}
