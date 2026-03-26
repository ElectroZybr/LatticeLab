#pragma once

#include <array>
#include <SFML/Graphics.hpp>

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

    float averageKineticEnegry() const;
    float averagePotentialEnergy() const;
    float fullAverageEnergy() const;

    void logAtomPos() const;
    void logBondList() const;

    int getSimStep() const { return sim_step; }
    void setNeighborListEnabled(bool enabled);
    bool isNeighborListEnabled() const { return useNeighborList_; }
    std::size_t neighborListRebuildCount() const { return neighborListRebuildCount_; }
    float averageStepsPerNeighborListRebuild() const;
    float recentAverageStepsPerNeighborListRebuild() const;
    int stepsSinceNeighborListRebuild() const;

    void setIntegrator(Integrator::Scheme scheme) { integrator.setScheme(scheme); }
    Integrator::Scheme getIntegrator() const { return integrator.getScheme(); }

    void save(const std::string_view path) const;
    void load(const std::string_view path);
    void clear();

    SimBox& sim_box;
    AtomStorage atomStorage;
    Integrator integrator;
    ForceField forceField;
    NeighborList neighborList;
private:
    int sim_step = 0;
    bool useNeighborList_ = true;
    std::size_t neighborListRebuildCount_ = 0;
    std::size_t neighborListRebuildIntervalsSum_ = 0;
    int lastNeighborListRebuildStep_ = -1;
    static constexpr std::size_t kRecentRebuildWindow = 8;
    std::array<std::size_t, kRecentRebuildWindow> recentRebuildIntervals_{};
    std::size_t recentRebuildIntervalCount_ = 0;
    std::size_t recentRebuildIntervalCursor_ = 0;

    bool checkNeighbor(Vec3f coords, float delta);
    void onNeighborListRebuild();
    void resetNeighborListStats();
};


