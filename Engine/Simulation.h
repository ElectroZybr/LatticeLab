#pragma once

#include <SFML/Graphics.hpp>

#include "physics/AtomData.h"
#include "physics/AtomStorage.h"
#include "physics/SpatialGrid.h"
#include "SimBox.h"
#include "physics/Integrator.h"
#include "physics/ForceField.h"

class Simulation {
public:
    Simulation(SimBox& sim_box);

    void update(float dt);

    void setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize = -1);

    void createRandomAtoms(AtomData::Type type, int quantity);
    bool createAtom(Vec3D start_coords, Vec3D start_speed, AtomData::Type type, bool fixed = false);
    bool removeAtom(std::size_t atomIndex);
    void addBond(std::size_t aIndex, std::size_t bIndex);

    double averageKineticEnegry() const;
    double averagePotentialEnergy() const;
    double fullAverageEnergy() const;

    void logEnergies() const;
    void logAtomPos() const;
    void logBondList() const;

    int getSimStep() const { return sim_step; }

    void setIntegrator(Integrator::Scheme scheme) { integrator.setScheme(scheme); }
    Integrator::Scheme getIntegrator() const { return integrator.getScheme(); }

    void save(const std::string_view path) const;
    void load(const std::string_view path);
    void clear();

    SimBox& sim_box;
    std::vector<Atom> atoms;
    AtomStorage atomStorage;
    Integrator integrator;
    ForceField forceField;
private:
    int sim_step = 0;

    bool checkNeighbor(Vec3D coords, float delta);
};

