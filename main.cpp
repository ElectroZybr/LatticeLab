#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <cmath>

#include <iostream>
#include <chrono>
#include <vector>

#include "imgui-SFML.h"

#include "Engine/physics/Atom.h"
#include "Engine/physics/SpatialGrid.h"

#include "Engine/Simulation.h"

#include "GUI/interface/interface.h"
#include "GUI/interface/panels/debug/view/DebugView.h"
#include "Engine/physics/Bond.h"
#include "Engine/utils/Timer.h"

#include "Engine/renderer/Renderer2D.h"

constexpr int WIGHT = 800;
constexpr int HEIGHT = 600;

constexpr int FPS = 60;
constexpr int LPS = 1;
constexpr double Dt = 0.01;

/* тестовые сцены, можно запускать в main и экспериментировать*/
void square15x15H(Simulation& simulation);
void crystal25x25H(Simulation& simulation);
void crystal100x100H(Simulation& simulation);
void diffusionTest(Simulation& simulation);

int main() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator", sf::State::Fullscreen);
    sf::Image icon;
    if (icon.loadFromFile("icon.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    Timer physicsTimer;
    Timer renderTimer;

    SimBox box(Vec3D(-25, -25, 0), Vec3D(25, 25, 6));
    Simulation simulation(window, box);

    IRenderer* renderer = new Renderer2D(window, simulation.getGameView(), simulation.getUiView());
    simulation.setRenderer(renderer);

    simulation.setCameraPos(0, 0);
    simulation.setCameraZoom(20);

    // simulation.drawGrid(true);
    simulation.drawBonds();
    // simulation.speedGradient();

    crystal25x25H(simulation);

    // simulation.render.speedGradientTurbo = true;
    Interface::pause = true;

    DebugView* debugSim = Interface::debugPanel.addView(DebugView("Симуляция", 
    {
        DebugSeries("Кинетическая энергия"),
        DebugValue ("Количество атомов"),
        DebugValue ("Шаги симуляции"),
        DebugValue ("Физика (мс)"),
        DebugValue ("Рендер (мс)"),
    }));

    DebugView* debugAtom = Interface::debugPanel.addView(DebugView("Атом",
    {
        DebugValue ("В разработке"),
    }));

    debugSim->add_data("Количество атомов", simulation.atoms.size());
    debugSim->add_data("Шаги симуляции", simulation.getSimStep());

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
    
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        simTmr += deltaTime;
        if (simTmr >= Dt/Interface::getSimulationSpeed()) { 
            physicsTimer.start();
            simulation.update(Dt);
            physicsTimer.stop();
            physics_time_ms_accum += physicsTimer.elapsedMilliseconds();
            physics_steps_per_second++;
            if (!Interface::getPause()) {
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
        simulation.pollEvents(); // Обработка событий окна
        if (shotTmr >= 1./FPS) {
            simulation.event(); // Обработка взаимодейсвия интерфейса с миром (перетаскивание атомов)
            if (auto result = Interface::fileDialog.popResult()) {
                switch (result->command) {
                    case SimCommand::Save: simulation.save(result->path); break;
                    case SimCommand::Load: simulation.load(result->path); break;
                }
            }
            renderTimer.start();
            simulation.renderShot(shotTmr);
            renderTimer.stop();
            render_time_ms_accum += renderTimer.elapsedMilliseconds();
            render_frames_per_second++;
            shotTmr = 0;

            debugSim->add_data("Кинетическая энергия", simulation.AverageEnegry());
            debugSim->add_data("Рендер (мс)", renderTimer.elapsedMilliseconds());
            debugSim->add_data("Физика (мс)", physicsTimer.elapsedMilliseconds());
            debugSim->add_data("Шаги симуляции", simulation.getSimStep());
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
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), randomUnitVector3D(0.5), 1);
        }
    }
}

void crystal25x25H(Simulation& simulation) {
    simulation.speedGradient();
    simulation.setSizeBox(Vec3D(-50, -50, simulation.sim_box.start.z), Vec3D(50, 50, simulation.sim_box.end.z));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            Atom* atom = simulation.createAtom(Vec3D(15+i*2.5, 15+j*2.5, 2), randomUnitVector3D(0.5), 1);
            atom->a = 2.0;
            atom->eps = 15;
        }
    }
}

void crystal100x100H(Simulation& simulation) {
    constexpr int N = 100;
    constexpr int size = 175;
    simulation.speedGradient();
    simulation.setSizeBox(Vec3D(-size, -size, simulation.sim_box.start.z), Vec3D(size, size, simulation.sim_box.end.z));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            Atom* atom = simulation.createAtom(Vec3D(15+i*2.5, 15+j*2.5, 2), randomUnitVector3D(0.5), 1);
            atom->a = 2.0;
            atom->eps = 15;
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
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), randomUnitVector3D(0.5), 1);
        }
    }
    for (int i = 0; i < 15; i++) {
        for (int j = 8; j < 15; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), randomUnitVector3D(0.5), 8);
        }
    }
}
