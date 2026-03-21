#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <cmath>

#include <iostream>
#include <chrono>
#include <string_view>

#include "imgui-SFML.h"

#include "Engine/physics/Atom.h"
#include "Engine/physics/SpatialGrid.h"

#include "GUI/io/manager/EventManager.h"
#include "GUI/io/keyboard/Keyboard.h"

#include "Engine/Simulation.h"

#include "GUI/interface/interface.h"
#include "GUI/interface/panels/debug/view/DebugView.h"
#include "Engine/physics/Bond.h"
#include "Engine/utils/Timer.h"

#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

#include "Engine/Tools.h"

#include "memory_monitor.h"

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

constexpr int FPS = 60;
constexpr int LPS = 1;
constexpr double Dt = 0.01;

/* тестовые сцены, можно запускать в main и экспериментировать*/
void square15x15H(Simulation& simulation);
void crystal25x25H(Simulation& simulation);
void crystal2dH(Simulation& simulation, int n = 15);
void crystal3dH(Simulation& simulation, int n = 15);
void diffusionTest(Simulation& simulation);

std::string_view schemeName(Integrator::Scheme s);

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator", sf::State::Fullscreen, settings);
    sf::Image icon;
    if (icon.loadFromFile("icon.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    Timer physicsTimer;
    Timer renderTimer;

    SimBox box(Vec3D(-25, -25, 0), Vec3D(25, 25, 6));
    Simulation simulation(window, box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);

    crystal25x25H(simulation);
    // crystal2dH(simulation, 150);
    // crystal3dH(simulation, 30);

    IRenderer* renderer = new Renderer2D(window, simulation.getGameView(), simulation.getUiView());
    renderer->camera.setPosition(0, 0);
    renderer->camera.setZoom(20);
    renderer->drawBonds = true;
    renderer->speedGradient = true;
    renderer->wallImage(box.start, box.end);
    // renderer->drawGrid = true;
    // renderer->speedGradientTurbo = true;
    EventManager::init(&window, &simulation.getUiView(), renderer, &simulation.sim_box, &simulation.atoms);
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


    // debugSim->add_data("Количество атомов", simulation.atoms.size());
    // debugSim->add_data("Шаги симуляции", simulation.getSimStep());

    // Atom* hydrogen_1 = simulation.createAtom(Vec3D(25.5, 25.86, 1), Vec3D(2, 0, 0), 1);
    // Atom* oxygen_1 = simulation.createAtom(Vec3D(25, 25, 1), Vec3D(0, 0, 0), 8);
    // Atom* hydrogen_2 = simulation.createAtom(Vec3D(26, 25, 1), Vec3D(0, 0, 0), 1);

    // simulation.addBond(hydrogen_1, oxygen_1);
    // simulation.addBond(hydrogen_2, oxygen_1);

    // simulation.createRandomAtoms(1, 50);
    // simulation.createRandomAtoms(8, 100);
    
    sf::Clock clock;
    double shotTmr = 0.0;
    double simTmr = 0.0;
    double logTmr = 0.0;

    auto duration = std::chrono::microseconds::zero();
    int i = 0;
    int sim_step = 0;
    int while_cycle_per_second = 0;
    double physics_time_ms_accum = 0.0;
    double render_time_ms_accum = 0.0;
    int physics_steps_per_second = 0;
    int render_frames_per_second = 0;
    float physics_steps_rate = 0.0f;
    
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        simTmr += deltaTime;
        if (simTmr >= Dt/Interface::getSimulationSpeed()) { 
            if (!Interface::getPause()) {
                physicsTimer.start();
                simulation.update(Dt);
                physicsTimer.stop();
                physics_time_ms_accum += physicsTimer.elapsedMilliseconds();
                physics_steps_per_second++;
                // if (simulation.getSimStep() < 12000) {
                //     simulation.setSizeBox(
                //         Vec3D(simulation.sim_box.start.x+0.001, simulation.sim_box.start.y+0.001, simulation.sim_box.start.z),
                //         Vec3D(simulation.sim_box.end.x-0.001, simulation.sim_box.end.y-0.001, simulation.sim_box.end.z));
                // }
                
                // if (simulation.getSimStep() > 8000 && simulation.getSimStep() < 8050) {
                //     simulation.setSizeBox(
                //         Vec3D(simulation.sim_box.start.x, simulation.sim_box.start.y, simulation.sim_box.start.z),
                //         Vec3D(simulation.sim_box.end.x-0.075, simulation.sim_box.end.y, simulation.sim_box.end.z));
                // }

                // if (simulation.getSimStep() > 4000 && simulation.getSimStep() < 100000)
                //     simulation.setSizeBox(
                //         Vec3D(simulation.sim_box.start.x-0.001, simulation.sim_box.start.y-0.001, simulation.sim_box.start.z),
                //         Vec3D(simulation.sim_box.end.x+0.001, simulation.sim_box.end.y+0.001, simulation.sim_box.end.z));
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
                        physicsTimer.start();
                        simulation.update(Dt);
                        physicsTimer.stop();
                        physics_time_ms_accum += physicsTimer.elapsedMilliseconds();
                        physics_steps_per_second++;
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
                        renderer = new Renderer2D(window,
                            simulation.getGameView(),
                            simulation.getUiView());
                        break;
                    case ToolsCommand::ToggleRenderer3D:
                        renderer = new Renderer3D(window,
                            simulation.getGameView(),
                            simulation.getUiView()
                        );
                        break;
                }
                Tools::setRenderer(renderer);
                EventManager::updateRenderer(renderer);
                delete oldRenderer;
            }

            renderTimer.start();
            renderer->drawShot(simulation.atoms, simulation.sim_box, shotTmr);
            renderTimer.stop();
            render_time_ms_accum += renderTimer.elapsedMilliseconds();
            render_frames_per_second++;
            shotTmr = 0;

            debugSim->add_data("Полная энергия", simulation.fullAverageEnergy());
            debugSim->add_data("Память (МБ)", MemoryMonitor::getRSS() / 1024.f / 1024.f);
            debugSim->add_data("Рендер (мс)", renderTimer.elapsedMilliseconds());
            debugSim->add_data("Физика (мс)", physicsTimer.elapsedMilliseconds());
            debugSim->add_data("Количество атомов", static_cast<float>(simulation.atoms.size()));
            debugSim->add_data("Шаги симуляции", simulation.getSimStep());
            debugSim->add_data("Шагов/с", physics_steps_rate);
            debugSim->add_data("Тип интегратора", schemeName(simulation.getIntegrator()));
        }

        logTmr += deltaTime;
        if (logTmr >= 1./LPS) {
            const double avg_physics_ms = (physics_steps_per_second > 0) ? (physics_time_ms_accum / physics_steps_per_second) : 0.0;
            const double avg_render_ms  = (render_frames_per_second > 0) ? (render_time_ms_accum / render_frames_per_second) : 0.0;
            std::cout << "[perf] loop/s: " << while_cycle_per_second
                      << " | phys/s: " << physics_steps_per_second
                      << " | phys avg ms: " << avg_physics_ms
                      << " | phys total ms: " << physics_time_ms_accum
                      << " | render/s: " << render_frames_per_second
                      << " | render avg ms: " << avg_render_ms
                      << " | render total ms: " << render_time_ms_accum
                      << std::endl;
            physics_steps_rate = static_cast<float>(physics_steps_per_second * LPS);
            while_cycle_per_second=0;
            physics_time_ms_accum = 0.0;
            render_time_ms_accum = 0.0;
            physics_steps_per_second = 0;
            render_frames_per_second = 0;
            // simulation.logEnergies();
            // simulation.logAtomPos();
            // simulation.logMousePos();
            // simulation.logBondList();
            logTmr = 0;
        }
        while_cycle_per_second++;
    }
    ImGui::SFML::Shutdown();
    delete renderer;

    return 0;
}

void square15x15H(Simulation& simulation) {
    simulation.setSizeBox(
        Vec3D(-25, -25, simulation.sim_box.start.z),
        Vec3D(25, 25, simulation.sim_box.end.z),
        5
    );
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), Vec3D::Random() * 0.5, 1);
        }
    }
}

void crystal25x25H(Simulation& simulation) {
    simulation.setSizeBox(Vec3D(-50, -50, simulation.sim_box.start.z), Vec3D(50, 50, simulation.sim_box.end.z));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            Atom* atom = simulation.createAtom(Vec3D(15+i*2.5, 15+j*2.5, 2), Vec3D::Random() * 0.5, 0);
        }
    }
}

void crystal3dH(Simulation& simulation, int n) {
    constexpr int padding = 3;
    const int sib_box_size = n * padding + padding;

    Vec3D start = Vec3D(-sib_box_size/2.f, -sib_box_size/2.f, -sib_box_size/2.f);
    simulation.setSizeBox(start, -start);

    for (int x = 1; x <= n; x++) {
        for (int y = 1; y <= n; y++) {
            for (int z = 1; z <= n; z++) {
                Vec3D pos(x, y, z);
                Atom* atom = simulation.createAtom(pos * padding, Vec3D::Random() * 0.5, 1);
            }
        }
    }
}

void crystal2dH(Simulation& simulation, int n) {
    constexpr int padding = 3;
    const int sib_box_size = n * padding + padding;

    Vec2D start = Vec2D(-sib_box_size/2.f, -sib_box_size/2.f);
    simulation.setSizeBox(Vec3D(start.x, start.y, simulation.sim_box.start.z), Vec3D(-start.x, -start.y, simulation.sim_box.end.z));

    for (int x = 1; x <= n; x++) {
        for (int y = 1; y <= n; y++) {
            Vec2D pos(x, y);
            Atom* atom = simulation.createAtom(Vec3D(pos.x, pos.y, 1) * padding, Vec3D::Random() * 0.5, 0);
        }
    }
}

void diffusionTest(Simulation& simulation) {
    simulation.setSizeBox(
        Vec3D(-25, -25, simulation.sim_box.start.z),
        Vec3D(25, 25, simulation.sim_box.end.z),
        5
    );
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 8; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), Vec3D::Random() * 0.5, 1);
        }
    }
    for (int i = 0; i < 15; i++) {
        for (int j = 8; j < 15; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), Vec3D::Random() * 0.5, 8);
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
