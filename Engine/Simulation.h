#pragma once

#include <SFML/Graphics.hpp>
#include <string>

#include "Engine/Consts.h"
#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/SimBox.h"
#include "Engine/metrics/EnergyMetrics.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"
#include "Engine/physics/ForceField.h"
#include "Engine/physics/Integrator.h"

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
    float simTimeNs() const { return sim_time_ns; }
    void restoreRuntimeState(int simStep, float simTimeNs) {
        sim_step = simStep;
        sim_time_ns = simTimeNs;
    }
    void setSceneTitle(std::string title) { sceneTitle_ = std::move(title); }
    const std::string& sceneTitle() const { return sceneTitle_; }
    void setSceneDescription(std::string description) { sceneDescription_ = std::move(description); }
    const std::string& sceneDescription() const { return sceneDescription_; }

    float averageKineticEnergyEv() const {
        refreshMetricsCache();
        return metricsCache_.averageKineticEnergyEv;
    }

    float averagePotentialEnergyEv() const {
        refreshMetricsCache();
        return metricsCache_.averagePotentialEnergyEv;
    }

    float fullAverageEnergyEv() const {
        refreshMetricsCache();
        return metricsCache_.fullAverageEnergyEv();
    }

    float fullEnegryPJ() const {
        return fullAverageEnergyEv() * atomStorage_.size() * Units::kEvToPJ;
    }

    float temperatureK() const {
        refreshMetricsCache();
        return metricsCache_.temperatureK();
    }

    float temperatureC() const {
        refreshMetricsCache();
        return metricsCache_.temperatureC();
    }

    float averageSpeedKmPerHour() const {
        refreshMetricsCache();
        return metricsCache_.averageSpeedKmPerHour();
    }

    void setBondFormationEnabled(bool enabled) { bondFormationEnabled_ = enabled; }
    bool isBondFormationEnabled() const { return bondFormationEnabled_; }
    void setLJEnabled(bool enabled) { forceField_.setLJEnabled(enabled); }
    bool isLJEnabled() const { return forceField_.isLJEnabled(); }
    void setCoulombEnabled(bool enabled) { forceField_.setCoulombEnabled(enabled); }
    bool isCoulombEnabled() const { return forceField_.isCoulombEnabled(); }
    void setGravity(const Vec3f& gravity) { forceField_.setGravity(gravity); }
    Vec3f getGravity() const { return forceField_.getGravity(); }
    void setNeighborListCutoff(float cutoff) { neighborList_.setCutoff(cutoff); }
    float getNeighborListCutoff() const { return neighborList_.cutoff(); }
    void setNeighborListSkin(float skin) { neighborList_.setSkin(skin); }
    float getNeighborListSkin() const { return neighborList_.skin(); }
    float getNeighborListRadius() const { return neighborList_.listRadius(); }

    AtomStorage& atoms() {
        invalidateMetricsCache();
        return atomStorage_;
    }
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
    void invalidateMetricsCache() const { metricsCacheValid_ = false; }
    void refreshMetricsCache() const;

    SimBox& sim_box_;
    AtomStorage atomStorage_;
    Integrator integrator;
    ForceField forceField_;
    NeighborList neighborList_;
    Bond::List bonds_;
    float Dt = 0.01f;
    int sim_step = 0;
    float sim_time_ns = 0.0f;
    bool bondFormationEnabled_ = false;
    std::string sceneTitle_;
    std::string sceneDescription_;
    mutable bool metricsCacheValid_ = false;
    mutable EnergyMetrics::Snapshot metricsCache_{};
};
