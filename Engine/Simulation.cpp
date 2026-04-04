#include <cmath>

#include "Simulation.h"
#include "io/SimulationStateIO.h"
#include "metrics/Profiler.h"
#include "physics/Bond.h"

#include "Engine/restrict.h"

Simulation::Simulation(SimBox& box)
    : sim_box_(box), integrator() {
    atomStorage_.reserve(250000);
    neighborList_.setParams(5.f, 1.f);
    forceField_.syncWalls(sim_box_);
}

void Simulation::setNeighborListEnabled(bool enabled) {
    if (useNeighborList_ == enabled) {
        return;
    }

    useNeighborList_ = enabled;
    if (!useNeighborList_) {
        neighborList_.clear();
    }
}

void Simulation::update() {
    PROFILE_SCOPE("Simulation::update");
    integrator.step(atomStorage_, sim_box_, forceField_, useNeighborList_ ? &neighborList_ : nullptr, Dt);
    if (useNeighborList_ && neighborList_.needsRebuild(atomStorage_)) {
        neighborList_.build(atomStorage_, sim_box_);
        neighborList_.recordRebuild(sim_step);
    }
    ++sim_step;
    sim_time_ns += Dt * Units::kTimeUnitToNs;
}

void Simulation::setSizeBox(Vec3f newSize, int cellSize) {
    const bool resized = sim_box_.setSizeBox(newSize, cellSize);
    if (resized) {
        forceField_.syncWalls(sim_box_);
        sim_box_.grid.rebuild(atomStorage_.xDataSpan(), atomStorage_.yDataSpan(), atomStorage_.zDataSpan());
        neighborList_.clear();
    }
}

bool Simulation::createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed) {
    atomStorage_.addAtom(start_coords, start_speed, type, fixed);
    sim_box_.grid.rebuild(atomStorage_.xDataSpan(), atomStorage_.yDataSpan(), atomStorage_.zDataSpan());
    return true;
}

bool Simulation::removeAtom(size_t atomIndex) {
    if (atomIndex >= atomStorage_.size()) {
        return false;
    }

    const size_t lastIndex = atomStorage_.size() - 1;

    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        if (it->aIndex == atomIndex || it->bIndex == atomIndex) {
            if (it->aIndex == atomIndex && it->bIndex != atomIndex && it->bIndex < atomStorage_.size()) {
                ++atomStorage_.valenceCount(it->bIndex);
            }
            if (it->bIndex == atomIndex && it->aIndex != atomIndex && it->aIndex < atomStorage_.size()) {
                ++atomStorage_.valenceCount(it->aIndex);
            }
            it = Bond::bonds_list.erase(it);
            continue;
        }

        if (atomIndex != lastIndex) {
            if (it->aIndex == lastIndex) {
                it->aIndex = atomIndex;
            }
            if (it->bIndex == lastIndex) {
                it->bIndex = atomIndex;
            }
        }

        ++it;
    }

    atomStorage_.removeAtom(atomIndex);
    sim_box_.grid.rebuild(atomStorage_.xDataSpan(), atomStorage_.yDataSpan(), atomStorage_.zDataSpan());
    return true;
}

void Simulation::addBond(size_t aIndex, size_t bIndex) {
    if (aIndex >= atomStorage_.size() || bIndex >= atomStorage_.size()) {
        return;
    }

    Bond::CreateBond(aIndex, bIndex, atomStorage_);
}

void Simulation::clear() {
    atomStorage_.clear();
    Bond::bonds_list.clear();
    sim_box_.grid.rebuild(atomStorage_.xDataSpan(), atomStorage_.yDataSpan(), atomStorage_.zDataSpan());
    neighborList_.clear();
    sim_step = 0;
}
