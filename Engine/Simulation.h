#pragma once

#include <SFML/Graphics.hpp>

#include "metrics/NeighborListMetrics.h"
#include "physics/AtomData.h"
#include "physics/AtomStorage.h"
#include "physics/SpatialGrid.h"
#include "SimBox.h"
#include "physics/Integrator.h"
#include "physics/ForceField.h"
#include "physics/NeighborList.h"

class Simulation {
public:
    Simulation(SimBox& sim_box);

    void update(float dt);

    void setSizeBox(Vec3f newStart, Vec3f newEnd, int cellSize = -1);

    void createRandomAtoms(AtomData::Type type, int quantity);
    bool createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed = false);
    bool removeAtom(std::size_t atomIndex);
    void addBond(std::size_t aIndex, std::size_t bIndex);

    void setIntegrator(Integrator::Scheme scheme) { integrator.setScheme(scheme); }
    Integrator::Scheme getIntegrator() const { return integrator.getScheme(); }
    
    // метрики
    float averageKineticEnegry() const;
    float averagePotentialEnergy() const;
    float fullAverageEnergy() const;
    int getSimStep() const { return sim_step; }
    void setNeighborListEnabled(bool enabled);
    bool isNeighborListEnabled() const { return useNeighborList_; }
    std::size_t neighborListRebuildCount() const { return neighborListMetrics_.rebuildCount(); }
    float averageStepsPerNeighborListRebuild() const;
    float recentAverageStepsPerNeighborListRebuild() const;
    int stepsSinceNeighborListRebuild() const;

    // io
    void save(const std::string_view path) const;
    void load(const std::string_view path);
    void clear();

    SimBox& sim_box;
    AtomStorage atomStorage;
    Integrator integrator;
    ForceField forceField;
    NeighborList neighborList;
private:
    friend class SimulationStateIO;

    int sim_step = 0;
    bool useNeighborList_ = true;
    NeighborListMetrics neighborListMetrics_;

    bool checkNeighbor(Vec3f coords, float delta);
};


