#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <cmath>

#include <iostream>
#include <functional>
#include <memory>
#include <chrono>

#include "imgui.h"
#include "imgui-SFML.h"

#include "Engine/physics/Atom.h"
#include "Engine/physics/SpatialGrid.h"
#include "interface.h"

#include "Engine/Simulation.h"

#define WIGHT   800
#define HEIGHT  600

#define FPS  60
#define LPS  30
#define Dt  0.01

#include <vector>

/* тестовые сцены, можете запускать в main и экспериментировать*/
void square15x15H(Simulation& simulation);
void crystal25x25H(Simulation& simulation);
void diffusionTest(Simulation& simulation);

int main() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator", sf::Style::Fullscreen);

    sf::Image icon;
    if (icon.loadFromFile("icon.png")) {
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }

    SimBox box(Vec3D(-250, -250, 0), Vec3D(250, 250, 3));
    Simulation simulation(window, box);
    simulation.setCameraPos(0, 0);
    simulation.setCameraZoom(20);

    // simulation.drawGrid(true);
    simulation.drawBonds();
    // simulation.speedGradient();

    crystal25x25H(simulation);
    // simulation.render.speedGradientTurbo = true;

    // Atom* hydrogen_1 = simulation.createAtom(Vec3D(50.5, 50.86, 1), Vec3D(2, 0, 0), 1);
    // Atom* oxygen_1 = simulation.createAtom(Vec3D(50, 50, 1), Vec3D(0, 0, 0), 8);
    // Atom* hydrogen_2 = simulation.createAtom(Vec3D(51, 50, 1), Vec3D(0, 0, 0), 1);

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
    
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        simTmr += deltaTime;
        if (simTmr >= Dt/Interface::getSimulationSpeed()) { 
            // auto start = std::chrono::high_resolution_clock::now();
            simulation.update(Dt);
            if (!Interface::getPause()) {
                // if (simulation.getSimStep() < 6000) {
                //     simulation.setSizeBox(
                //         Vec3D(simulation.sim_box.start.x+0.005, simulation.sim_box.start.y+0.005, simulation.sim_box.start.z),
                //         Vec3D(simulation.sim_box.end.x-0.005, simulation.sim_box.end.y-0.005, simulation.sim_box.end.z));
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
            // if (!Interface::getPause()) {
            //     simulation.setCameraZoom(simulation.render.camera.getZoom()*(1.f + 0.1 * -0.01));
            //     sim_step++;
            // }

            // shotTmr = 0;
            // auto end = std::chrono::high_resolution_clock::now();
            // duration += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            // i++;
            simTmr = 0;
        }

        shotTmr += deltaTime;
        if (shotTmr >= 1./FPS) {
            simulation.event();
            simulation.renderShot(shotTmr);
            shotTmr = 0;
        }

        logTmr += deltaTime;
        if (logTmr >= 1./LPS) {
            // simulation.logEnergies();
            // simulation.logAtomPos();
            // simulation.logMousePos();
            // simulation.logBondList();
            logTmr = 0;
        }
    }
    ImGui::SFML::Shutdown();
    
    return 0;
}

void square15x15H(Simulation& simulation) {
    simulation.setSizeBox(Vec3D(-25, -25, 0), Vec3D(25, 25, 3), 5);
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            simulation.createAtom(Vec3D(4+i*3, 4+j*3, 1), randomUnitVector3D(0.5), 1);
        }
    }
}

void crystal25x25H(Simulation& simulation) {
    simulation.speedGradient();
    simulation.setSizeBox(Vec3D(-50, -50, 0), Vec3D(50, 50, 3), 5);
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 25; j++) {
            Atom* atom = simulation.createAtom(Vec3D(4+i*2.5, 4+j*2.5, 1), randomUnitVector3D(0.5), 1);
            atom->a = 2.0;
            atom->eps = 15;
        }
    }
}

void diffusionTest(Simulation& simulation) {
    simulation.setSizeBox(Vec3D(-25, -25, 0), Vec3D(25, 25, 3), 5);
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
