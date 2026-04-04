#pragma once

#include <SFML/Graphics.hpp>

#include "physics/AtomData.h"
#include "physics/AtomStorage.h"
#include "physics/Bond.h"
#include "SimBox.h"
#include "physics/Integrator.h"
#include "physics/ForceField.h"
#include "NeighborSearch/NeighborList.h"
#include "Consts.h"
#include "metrics/EnergyMetrics.h"

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
    double simTimeNs() const { return sim_time_ns; }
    double fullEnegryPJ() const { return EnergyMetrics::fullAverageEnergy(atomStorage_) * static_cast<double>(atomStorage_.size()) * Units::kEvToPJ; }

    void setBondFormationEnabled(bool enabled) { bondFormationEnabled_ = enabled; }
    bool isBondFormationEnabled() const { return bondFormationEnabled_; }
    void setGravity(const Vec3f& gravity) { forceField_.setGravity(gravity); }
    Vec3f getGravity() const { return forceField_.getGravity(); }
    void setNeighborListCutoff(float cutoff) { neighborList_.setCutoff(cutoff); }
    float getNeighborListCutoff() const { return neighborList_.cutoff(); }
    void setNeighborListSkin(float skin) { neighborList_.setSkin(skin); }
    float getNeighborListSkin() const { return neighborList_.skin(); }
    float getNeighborListRadius() const { return neighborList_.listRadius(); }

    AtomStorage& atoms() { return atomStorage_; }
    const AtomStorage& atoms() const { return atomStorage_; }
    SimBox& box() { return sim_box_; }
    const SimBox& box() const { return sim_box_; }
    ForceField& forceField() { return forceField_; }
    const ForceField& forceField() const { return forceField_; }
    NeighborList& neighborList() { return neighborList_; }
    const NeighborList& neighborList() const { return neighborList_; }
    Bond::List& bonds() { return bonds_; }
    const Bond::List& bonds() const { return bonds_; }

    void clear();
private:
    friend class SimulationStateIO;
    StepData makeStepData();

    SimBox& sim_box_;
    AtomStorage atomStorage_;
    Integrator integrator;
    ForceField forceField_;
    NeighborList neighborList_;
    Bond::List bonds_;
    float Dt = 0.01;
    int sim_step = 0;
    double sim_time_ns = 0;
    bool bondFormationEnabled_ = true;
};
