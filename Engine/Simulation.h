#pragma once

#include <SFML/Graphics.hpp>

#include "physics/Atom.h"
#include "physics/SpatialGrid.h"
#include "SimBox.h"
#include "Rendering/BaseRenderer.h"
#include "physics/Integrator.h"
#include "physics/ForceField.h"

class Simulation {
public:
    Simulation(sf::RenderWindow& window, SimBox& sim_box);
    void setRenderer(IRenderer* r);

    void update(float dt);

    void renderShot(float dt);
    void setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize = -1);

    void createRandomAtoms(int type, int quantity);
    Atom* createAtom(Vec3D start_coords, Vec3D start_speed, int type, bool fixed = false);
    void addBond(Atom* a1, Atom* a2);

    double averageKineticEnegry() const;
    double averagePotentialEnergy() const;
    double fullAverageEnergy() const;

    void logEnergies() const;
    void logAtomPos() const;
    void logMousePos() const;
    void logBondList() const;

    int getSimStep() const { return sim_step; }

    sf::View& getGameView() { return gameView; }
    sf::View& getUiView()   { return uiView;   }

    void save(const std::string_view path) const;
    void load(const std::string_view path);
    void clear();

    SimBox& sim_box;
    IRenderer* render = nullptr;
    std::vector<Atom> atoms;
private:
    sf::RenderWindow& window;
    sf::View gameView;
    sf::View uiView;
    Integrator step;
    ForceField forceField;

    int sim_step = 0;

    bool checkNeighbor(Vec3D coords, float delta);
};

Vec2D randomUnitVector2D();
Vec3D randomUnitVector3D(double amplitude = 1.0);
double randomInRange(int range);

