#pragma once

#include <SFML/Graphics.hpp>

#include "physics/AtomData.h"
#include "physics/AtomStorage.h"
#include "SimBox.h"
#include "physics/Integrator.h"
#include "physics/ForceField.h"
#include "NeighborSearch/NeighborList.h"

class Simulation {
public:
    Simulation(SimBox& sim_box);

    void update();

    void setSizeBox(Vec3f newSize, int cellSize = -1);

    bool createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed = false);
    bool removeAtom(size_t atomIndex);
    void addBond(size_t aIndex, size_t bIndex);

    void setDt(float dt) { Dt = dt; }
    float getDt() const { return Dt; }
    void setIntegrator(Integrator::Scheme scheme) { integrator.setScheme(scheme); }
    Integrator::Scheme getIntegrator() const { return integrator.getScheme(); }
    void setMaxParticleSpeed(float maxSpeed) { integrator.setMaxParticleSpeed(maxSpeed); }
    float getMaxParticleSpeed() const { return integrator.maxParticleSpeed(); }
    void setAccelDamping(float accelDamping) { integrator.setAccelDamping(accelDamping); }
    float getAccelDamping() const { return integrator.accelDamping(); }
    
    int getSimStep() const { return sim_step; }
    void setNeighborListEnabled(bool enabled);
    bool isNeighborListEnabled() const { return useNeighborList_; }

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

    float Dt = 0.01;
    int sim_step = 0;
    bool useNeighborList_ = true;
};
