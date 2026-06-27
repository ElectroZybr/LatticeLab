#include "Application.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "App/AppActions.h"
#include "App/CreateWindow.h"
#include "App/WindowController.h"
#include "Lattice/Scripting/LuaState.h"
#include "Lattice/Log.hpp"
#include "App/UserSettings.h"
#include "App/viewport/SceneViewport.h"
#include "App/interaction/ToolsManager.h"
#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "GUI/interface/interface.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/manager/EventManager.h"
#include "Rendering/backend/WGPUContext.h"
#include "capture/CaptureActions.h"
#include "capture/CaptureController.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"
#include "Lattice/Engine/pluginLoader.hpp"

using Clock = std::chrono::high_resolution_clock;

constexpr int FPS = 60;
constexpr int LPS = 20;

namespace {
    const std::filesystem::path MoleculesPath = std::filesystem::path("Mods") / "Base" / "Molecules";
    const std::filesystem::path pluginsPath = std::filesystem::path("Plugins");

    uint32_t makeXYZStepInterval(float simulationStepsPerSecond, int captureFps) {
        const float sanitizedStepsPerSecond = std::max(simulationStepsPerSecond, 1.0f);
        const int sanitizedCaptureFps = std::max(captureFps, 1);
        return std::max(1u, static_cast<uint32_t>(std::lround(sanitizedStepsPerSecond / static_cast<float>(sanitizedCaptureFps))));
    }

    void presentInitializedWindow(GLFWwindow* window, SceneViewport& renderer, Lattice::Simulation& simulation, Interface& appInterface,
                                  const DebugViews& debugViews) {
        renderer.renderFrame(simulation, appInterface, debugViews);
        glfwShowWindow(window);
        glfwFocusWindow(window);
    }

    void loadBaseMoleculeTemplates(Lattice::Simulation& simulation) {
        size_t loadedCount = 0;

        if (!std::filesystem::exists(MoleculesPath) || !std::filesystem::is_directory(MoleculesPath)) {
            Log::warning("Application", "Molecule templates directory is missing: {}", MoleculesPath.string());
            return;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(MoleculesPath)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".pdb") {
                continue;
            }

            const std::string moleculeName = entry.path().stem().string();
            if (moleculeName.empty()) {
                continue;
            }

            try {
                simulation.loadMoleculeTemplate(moleculeName, entry.path());
                ++loadedCount;
            }
            catch (const std::exception& e) {
                Log::warning("Application", "Failed to load molecule template '{}': {}", entry.path().string(), e.what());
            }
        }

        Log::info("Application", "Loaded {} molecule template(s)", loadedCount);
    }
}

int Application::run() {
    PluginLoader pluginLoader(pluginsPath);
    UserSettings userSettings;
    {
        LogScope applicationScope("Application", "LatticeLab starting...", "Initialized");

        {
            LogScope settingsScope("Settings", "Loading user settings", "User settings loaded");
            userSettings = UserSettingsIO::load();
        }

        GLFWwindow* window = nullptr;
        {
            LogScope windowScope("Window", "Creating main window", "Window created");
            window = createWindow(userSettings.windowState);
            if (!window) {
                windowScope.cancel();
                applicationScope.cancel();
                Log::fatal("Window", "Failed to create main window");
                return EXIT_FAILURE;
            }
        }
        WindowController::init(window, userSettings.windowState);

        const auto [width, height] = WindowController::framebufferSize();
        {
            LogScope rendererScope("Renderer", "Initializing", "Renderer initialized");
            rendererScope.step("Creating WebGPU context");
            WGPUContext::instance().init(window, width, height);
        }

        Lattice::Simulation simulation;
        {
            LogScope simulationScope("Simulation", "Initializing", "Simulation initialized");
            simulationScope.step("Loading molecule templates");
            loadBaseMoleculeTemplates(simulation);
            simulationScope.step("Creating world");
            simulation.createWorld(glm::vec3(120.0f, 120.0f, 10.0f));
        }
        Lattice::LuaState luaState;
        luaState.bindSimulation(simulation);

        CaptureController captureController;
        const SceneViewport::RendererType initialRendererType =
            userSettings.rendererUse3D ? SceneViewport::RendererType::Renderer3D : SceneViewport::RendererType::Renderer2D;
        SceneViewport renderer(initialRendererType, captureController);
        renderer.syncScene(simulation);

        Interface appInterface(window, simulation, renderer.rendererHandle(), captureController);
        appInterface.toolsPanel.setRendererType(renderer.renderer().camera.getMode() == Camera::Mode::Mode2D ? RendererType::Renderer2D : RendererType::Renderer3D);

        AppActions::Handler appActions(window, captureController, simulation, renderer, appInterface.state());
        CaptureActions::Handler captureActions(captureController);
        {
            LogScope interfaceScope("Interface", "Initializing", "Interface initialized");
            interfaceScope.step("Creating ImGui context");
            if (appInterface.init() != EXIT_SUCCESS) {
                interfaceScope.cancel();
                applicationScope.cancel();
                Log::fatal("Interface", "Failed to initialize application interface");
                return EXIT_FAILURE;
            }
        }
        appInterface.setUiScaleMultiplier(userSettings.interfaceScale);
        EventManager::init(window, renderer.rendererHandle(), appInterface);
        ToolsManager::init(window, simulation, renderer.rendererHandle(), appInterface);
        const DebugViews debugViews = createDebugViews(appInterface.debugPanel);

        // загрузка пользовательских настроек
        captureController.setSettings(userSettings.captureSettings);
        captureController.setOutputDirectory(userSettings.captureOutputDirectory);
        appInterface.setScenesDirectory(userSettings.scenesDirectory);
        appInterface.ioPanel.setSceneCatalogView(static_cast<IOPanel::SceneCatalogView>(userSettings.sceneCatalogView));
        renderer.renderer().getRenderData(0).drawAtoms = userSettings.rendererDrawAtoms;
        renderer.renderer().getRenderData(0).drawGrid = userSettings.rendererDrawGrid;
        renderer.renderer().getRenderData(0).drawVectorField = userSettings.rendererDrawVectorField;
        renderer.renderer().getRenderData(0).drawFieldArrows = userSettings.rendererDrawFieldArrows;
        renderer.renderer().getRenderData(0).drawFieldContours = userSettings.rendererDrawFieldContours;
        renderer.renderer().getRenderData(0).fieldAutoScale = userSettings.rendererFieldAutoScale;
        renderer.renderer().getRenderData(0).fieldPotentialScale = userSettings.rendererFieldPotentialScale;
        renderer.renderer().getRenderData(0).fieldCellSize = userSettings.rendererFieldCellSize;
        renderer.renderer().getRenderData(0).fieldSmoothing = userSettings.rendererFieldSmoothing;
        renderer.renderer().getRenderData(0).fieldContourStep = userSettings.rendererFieldContourStep;
        simulation.world().setVectorFieldCellSize(userSettings.rendererFieldCellSize);
        renderer.renderer().getRenderData(0).drawBonds = userSettings.rendererDrawBonds;
        renderer.renderer().getRenderData(0).drawBox = userSettings.rendererDrawBox;
        renderer.renderer().getRenderData(0).drawMemoryOrder = userSettings.rendererDrawMemoryOrder;
        renderer.renderer().getRenderData(0).speedColorMode = userSettings.rendererSpeedColorMode;
        renderer.renderer().getRenderData(0).speedGradientMax = userSettings.rendererSpeedGradientMax;
        simulation.setIntegrator(userSettings.simulationIntegrator);
        simulation.world().setBondFormationEnabled(userSettings.simulationBondFormationEnabled);
        simulation.world().setLJEnabled(userSettings.simulationLJEnabled);
        simulation.world().setCoulombEnabled(userSettings.simulationCoulombEnabled);
        simulation.world().setCoulombLongRangeEnabled(userSettings.simulationCoulombLongRangeEnabled);
        appInterface.state().simulationSpeed = 100.0f;
        appInterface.state().pause = true;

        simulation.world().setVectorFieldSlice(static_cast<int>(simulation.world().getWorldSize().z * 0.5f));

        renderer.syncScene(simulation);

        auto startTime = Clock::now();
        double renderAccum = 0.0;
        double physicsAccum = 0.0;
        double debugRefreshAccum = 0.0;
        double statusAccum = 0.0;
        uint64_t lastStatusStep = simulation.getSimStep();

        constexpr double renderInterval = 1.0 / FPS;
        constexpr double debugRefreshInterval = 1.0 / LPS;
        constexpr double statusInterval = 2.0;

        renderer.setScreenSize(width, height);
        renderer.resetView();
        UiState& uiState = appInterface.state();

        presentInitializedWindow(window, renderer, simulation, appInterface, debugViews);
        applicationScope.finish();

        while (!glfwWindowShouldClose(window)) {
            Profiler::instance().beginFrame();

            auto currentTime = Clock::now();
            const float deltaTime = std::chrono::duration<float>(currentTime - startTime).count();
            startTime = currentTime;

            physicsAccum += deltaTime;
            renderAccum += deltaTime;
            debugRefreshAccum += deltaTime;
            statusAccum += deltaTime;

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
                    appActions.updateSimulationStep(simulation);
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

            // обновление debug-счетчиков и служебных панелей
            if (debugRefreshAccum >= debugRefreshInterval) {
                debugRefreshAccum -= debugRefreshInterval;
                refreshSimulationDebugViews(debugViews, simulation);
            }

            // периодический статус приложения в логах
            if (statusAccum >= statusInterval) {
                const uint64_t currentStep = simulation.getSimStep();
                const uint64_t advancedSteps = currentStep - lastStatusStep;
                const double averageUps = advancedSteps / statusAccum;

                Log::info(
                    "Simulation",
                    "Status: step={} atoms={} paused={} speed={} UPS={:.1f}",
                    currentStep,
                    simulation.world().getAtomStorage().size(),
                    uiState.pause,
                    uiState.simulationSpeed,
                    averageUps);

                lastStatusStep = currentStep;
                statusAccum = 0.0;
            }
        }

        Log::action("Application", "Shutting down...");
        captureController.stop();
        const UserSettings::WindowState windowState = WindowController::snapshot();
        {
            LogScope settingsScope("Settings", "Saving user settings", "User settings saved");
            UserSettingsIO::save(UserSettings{
            .captureOutputDirectory = captureController.outputDirectory(),
            .scenesDirectory = appInterface.scenesDirectory(),
            .sceneCatalogView = static_cast<UserSettings::SceneCatalogView>(appInterface.ioPanel.sceneCatalogView()),
            .captureSettings = captureController.settings(),
            .windowState = windowState,
            .rendererUse3D = renderer.renderer().camera.getMode() != Camera::Mode::Mode2D,
            .rendererDrawAtoms = renderer.renderer().getRenderData(0).drawAtoms,
            .rendererDrawGrid = renderer.renderer().getRenderData(0).drawGrid,
            .rendererDrawVectorField = renderer.renderer().getRenderData(0).drawVectorField,
            .rendererDrawFieldArrows = renderer.renderer().getRenderData(0).drawFieldArrows,
            .rendererDrawFieldContours = renderer.renderer().getRenderData(0).drawFieldContours,
            .rendererFieldAutoScale = renderer.renderer().getRenderData(0).fieldAutoScale,
            .rendererDrawBonds = renderer.renderer().getRenderData(0).drawBonds,
            .rendererDrawBox = renderer.renderer().getRenderData(0).drawBox,
            .rendererDrawMemoryOrder = renderer.renderer().getRenderData(0).drawMemoryOrder,
            .interfaceScale = appInterface.uiScaleMultiplier(),
            .rendererSpeedColorMode = renderer.renderer().getRenderData(0).speedColorMode,
            .rendererSpeedGradientMax = renderer.renderer().getRenderData(0).speedGradientMax,
            .rendererFieldPotentialScale = renderer.renderer().getRenderData(0).fieldPotentialScale,
            .rendererFieldCellSize = renderer.renderer().getRenderData(0).fieldCellSize,
            .rendererFieldSmoothing = renderer.renderer().getRenderData(0).fieldSmoothing,
            .rendererFieldContourStep = renderer.renderer().getRenderData(0).fieldContourStep,
            .simulationIntegrator = std::string(simulation.getIntegrator()),
            .simulationBondFormationEnabled = simulation.world().isBondFormationEnabled(),
            .simulationLJEnabled = simulation.isLJEnabled(),
            .simulationCoulombEnabled = simulation.isCoulombEnabled(),
            .simulationCoulombLongRangeEnabled = simulation.world().isCoulombLongRangeEnabled(),
            });
        }
        appInterface.shutdown();
        Log::ok("Application", "Shutdown complete");
        return 0;
    }

    return 0;
}
