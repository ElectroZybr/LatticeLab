#include <cmath>
#include <iostream>
#include <string_view>

#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include "imgui-SFML.h"

#include "GUI/io/manager/EventManager.h"
#include "GUI/io/keyboard/Keyboard.h"

#include "Engine/Simulation.h"

#include "GUI/interface/interface.h"
#include "GUI/interface/panels/debug/view/DebugView.h"
#include "Engine/utils/Timer.h"

#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

#include "Engine/Tools.h"

#include "memory_monitor.h"

constexpr int FPS = 60;
constexpr int LPS = 20;
constexpr double Dt = 0.01;

/* тестовые сцены, можно запускать в main и экспериментировать*/
void crystal(Simulation& simulation, int n, Atom::Type type, bool is3d, double padding = 3, double margin = 15);
void diffusionTest(Simulation& simulation);


struct PerSecondCounter {
    Timer timer;
    double time_ms_accum = 0.0;
    int steps_per_second = 0;
    float rate = 0.0f;

    void tick(double elapsed_ms) {
        time_ms_accum += elapsed_ms;
        steps_per_second++;
    }

    double avgMs() const {
        return steps_per_second > 0 ? time_ms_accum / steps_per_second : 0.0;
    }

    void reset() {
        time_ms_accum = 0.0;
        rate = steps_per_second * LPS;
        steps_per_second = 0;
    }
};

std::string_view schemeName(Integrator::Scheme s);

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator", sf::State::Fullscreen, settings);
    sf::Image icon;
    if (icon.loadFromFile("icon.png")) { window.setIcon(icon.getSize(), icon.getPixelsPtr()); }

    SimBox box(Vec3D(-25, -25, 0), Vec3D(25, 25, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);

    sf::View gameView = window.getDefaultView();
    sf::View uiView = window.getDefaultView();

    Interface::init(window);
    Tools::init(&window, &gameView, &box.grid, &box,
    [&](Vec3D coords, Vec3D speed, Atom::Type type, bool fixed) {
        return simulation.createAtom(coords, speed, type, fixed);
    });

    crystal(simulation, 20, Atom::Type::Z, false);

    IRenderer* renderer = new Renderer2D(window, gameView, uiView);
    renderer->camera.setPosition(0, 0);
    renderer->camera.setZoom(15);
    renderer->drawBonds = true;
    renderer->speedGradient = true;
    renderer->wallImage(box.start, box.end);
    // renderer->drawGrid = true;
    // renderer->speedGradientTurbo = true;

    EventManager::init(&window, &uiView, renderer, &simulation.sim_box, &simulation.atoms);
    Tools::setRenderer(renderer);

    Interface::pause = true;

    DebugView* debugSim = Interface::debugPanel.addView(DebugView("Симуляция", 
    {
        DebugValue ("Память (МБ)"),
        DebugValue ("Рендер (мс)"),
        DebugValue ("Физика (мс)"),
        DebugValue ("Тип интегратора"),
        DebugValue ("Шаги симуляции"),
        DebugValue ("Шагов/с"),
        DebugValue ("Количество атомов"),
        DebugSeries("Полная энергия"),
    }));

    DebugView* debugAtom = Interface::debugPanel.addView(DebugView("Атом",
    {
        DebugValue ("В разработке"),
    }));

    debugSim->add_data("Память (МБ)", MemoryMonitor::getRSS() / 1024.f / 1024.f);
    debugSim->add_data("Тип интегратора", schemeName(simulation.getIntegrator()));

    sf::Clock clock;
    double shotTmr = 0.0;
    double simTmr = 0.0;
    double logTmr = 0.0;

    PerSecondCounter physicsCounter;
    PerSecondCounter renderCounter;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        simTmr += deltaTime;
        if (simTmr >= Dt/Interface::getSimulationSpeed()) { 
            if (!Interface::getPause()) {
                physicsCounter.timer.start();
                simulation.update(Dt);
                physicsCounter.timer.stop();
                physicsCounter.tick(physicsCounter.timer.elapsedMilliseconds());
            }
            simTmr = 0;
        }

        shotTmr += deltaTime;
        EventManager::poll();
        if (shotTmr >= 1./FPS) {
            EventManager::frame(shotTmr);

            Interface::Update();

            if (auto result = Keyboard::popResult()) {
                switch (result.value()) {
                    case KeyboardCommand::StepPhysics:
                    {
                        physicsCounter.timer.start();
                        simulation.update(Dt);
                        physicsCounter.timer.stop();
                        physicsCounter.tick(physicsCounter.timer.elapsedMilliseconds());
                        break;
                    }
                }
            }

            if (auto result = Interface::fileDialog.popResult()) {
                switch (result->command) {
                    case FileDialogCommand::Save: simulation.save(result->path); break;
                    case FileDialogCommand::Load: simulation.load(result->path); break;
                }
            }

            if (auto result = Interface::toolsPanel.popResult()) {
                IRenderer* oldRenderer = renderer;
                switch (result.value()) {
                    case ToolsCommand::ToggleRenderer2D:
                        renderer = new Renderer2D(window, gameView, uiView);
                        break;
                    case ToolsCommand::ToggleRenderer3D:
                        renderer = new Renderer3D(window, gameView, uiView);
                        break;
                }
                renderer->drawBonds = oldRenderer->drawBonds;
                renderer->speedGradient = oldRenderer->speedGradient;
                renderer->speedGradientTurbo = oldRenderer->speedGradientTurbo;
                renderer->wallImage(box.start, box.end);
                Tools::setRenderer(renderer);
                EventManager::updateRenderer(renderer);
                delete oldRenderer;
            }

            renderCounter.timer.start();
            renderer->drawShot(simulation.atoms, simulation.sim_box, shotTmr);
            renderCounter.timer.stop();
            renderCounter.tick(renderCounter.timer.elapsedMilliseconds());
            shotTmr = 0;

            debugSim->add_data("Полная энергия", simulation.fullAverageEnergy());
        }

        logTmr += deltaTime;
        if (logTmr >= 1. / LPS) {
            debugSim->add_data("Память (МБ)", MemoryMonitor::getRSS() / 1024.f / 1024.f);
            debugSim->add_data("Рендер (мс)", static_cast<float>(renderCounter.avgMs()));
            debugSim->add_data("Физика (мс)", static_cast<float>(physicsCounter.avgMs()));
            debugSim->add_data("Количество атомов", static_cast<float>(simulation.atoms.size()));
            debugSim->add_data("Шаги симуляции", simulation.getSimStep());
            debugSim->add_data("Шагов/с", physicsCounter.rate);
            debugSim->add_data("Тип интегратора", schemeName(simulation.getIntegrator()));

            physicsCounter.reset();
            renderCounter.reset();
            logTmr = 0;
        }
    }
    ImGui::SFML::Shutdown();
    delete renderer;

    return 0;
}

void crystal(Simulation& simulation, int n, Atom::Type type, bool is3d, double padding, double margin) {
    const int sib_box_size = n * padding + padding + 2.0*margin;
    const double half = sib_box_size / 2.0;

    simulation.setSizeBox(
        Vec3D(-half, -half, is3d ? -half : simulation.sim_box.start.z),
        Vec3D( half,  half, is3d ?  half : simulation.sim_box.end.z)
    );

    const int zMax = is3d ? n : 1;
    const Vec3D vecMargin(margin, margin, is3d? margin : 0);
    for (int x = 1; x <= n; x++)
        for (int y = 1; y <= n; y++)
            for (int z = 1; z <= zMax; z++)
                simulation.createAtom( Vec3D(x, y, z) * padding + vecMargin, Vec3D::Random() * 0.5, type);
}

void diffusionTest(Simulation& simulation) {
    simulation.setSizeBox(
        Vec3D(-25, -25, simulation.sim_box.start.z),
        Vec3D(25, 25, simulation.sim_box.end.z),
        5
    );
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 8; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), Vec3D::Random() * 0.5, Atom::Type::H);
        }
    }
    for (int i = 0; i < 15; i++) {
        for (int j = 8; j < 15; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), Vec3D::Random() * 0.5, Atom::Type::O);
        }
    }
}

std::string_view schemeName(Integrator::Scheme s) {
    switch (s) {
        case Integrator::Scheme::Verlet:   return "Velocity Verlet";
        case Integrator::Scheme::KDK:      return "KDK (Kick-Drift-Kick)";
        case Integrator::Scheme::RK4:      return "Runge-Kutta 4";
        case Integrator::Scheme::Langevin: return "Langevin";
    }
    return "Unknown";
}
