#include "Application.h"
#include "AppActions.h"
#include "AppSignals.h"
#include "CreateWindow.h"
#include "capture/FrameRecorder.h"
#include "debug/CreateDebugPanels.h"
#include "debug/DebugRuntime.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>

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

namespace {
std::filesystem::path makeCaptureOutputPath() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream datePrefix;
    datePrefix << std::put_time(&localTime, "%Y-%m-%d");

    const std::filesystem::path capturesDir = "captures";
    std::filesystem::create_directories(capturesDir);

    const std::string prefix = datePrefix.str() + "_";
    int nextIndex = 1;

    for (const auto& entry : std::filesystem::directory_iterator(capturesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path path = entry.path();
        if (path.extension() != ".mp4") {
            continue;
        }

        const std::string stem = path.stem().string();
        if (!stem.starts_with(prefix)) {
            continue;
        }

        const std::string suffix = stem.substr(prefix.size());
        try {
            nextIndex = std::max(nextIndex, std::stoi(suffix) + 1);
        } catch (...) {
        }
    }

    return capturesDir / (prefix + std::to_string(nextIndex) + ".mp4");
}

}

int Application::run() {
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) {
        return EXIT_FAILURE;
    }
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    SimBox box(Vec3f(50, 50, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 500, AtomData::Type::Z, false);

    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView, simulation.box());
    renderer->setAtomStorage(&simulation.atoms());
    renderer->setBondStorage(&simulation.bonds());
    renderer->drawBonds = true;
    renderer->speedColorMode = IRenderer::SpeedColorMode::GradientClassic;

    AppActions::init(simulation, renderer, window, gameView);
    Interface::init(window, simulation, renderer);
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
    FrameRecorder frameRecorder;
    RendererCapture rendererCapture;
    Signals::ScopedConnection captureToggleConnection(
        AppSignals::UI::ToggleCapture.connect([&]() {
            if (frameRecorder.isRecording()) {
                frameRecorder.stop();
            } else {
                frameRecorder.start(makeCaptureOutputPath());
            }
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
        Interface::captureRecording = frameRecorder.isRecording();
        Interface::captureFrameCount = frameRecorder.savedFrameCount();

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

            if (frameRecorder.isRecording()) {
                CapturedFrame frame = rendererCapture.captureRGBA_PBO(window);
                if (!frame.empty()) {
                    frameRecorder.submit(std::move(frame));
                }
            }

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

    if (window.setActive(true)) {
        CapturedFrame pendingFrame = rendererCapture.consumePendingFrame();
        if (!pendingFrame.empty() && frameRecorder.isRecording()) {
            frameRecorder.submit(std::move(pendingFrame));
        }
    }
    frameRecorder.stop();
    Interface::shutdown();
    return 0;
}
