#include <cmath>

#include "Simulation.h"
#include "io/SimulationStateIO.h"
#include "metrics/Profiler.h"
#include "physics/Bond.h"

#include "Engine/restrict.h"

Simulation::Simulation(SimBox& box)
    : sim_box(box), integrator() {
    atomStorage.reserve(250000);
    neighborList.setParams(5.f, 1.f); // параметры отсечки и скин для NL
    forceField.syncWalls(sim_box);
}

void Simulation::setNeighborListEnabled(bool enabled) {
    if (useNeighborList_ == enabled) {
        return;
    }

    useNeighborList_ = enabled;
    if (!useNeighborList_) {
        neighborList.clear();
    }
}

void Simulation::update() {
    PROFILE_SCOPE("Simulation::update");
    integrator.step(atomStorage, sim_box, forceField, useNeighborList_ ? &neighborList : nullptr, Dt);
    if (useNeighborList_ && neighborList.needsRebuild(atomStorage)) {
        neighborList.build(atomStorage, sim_box);
        neighborList.recordRebuild(sim_step);
    }
    ++sim_step;
    sim_time_fs += Dt * kTimeUnitToFs;
}

void Simulation::setSizeBox(Vec3f newSize, int cellSize) {
    const bool resized = sim_box.setSizeBox(newSize, cellSize);
    if (resized) {
        forceField.syncWalls(sim_box);
        sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
        neighborList.clear();
    }
}

bool Simulation::createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed) {
    atomStorage.addAtom(start_coords, start_speed, type, fixed);
    sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
    return true;
}

bool Simulation::removeAtom(size_t atomIndex) {
    if (atomIndex >= atomStorage.size()) {
        return false;
    }

    const size_t lastIndex = atomStorage.size() - 1;

    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        if (it->aIndex == atomIndex || it->bIndex == atomIndex) {
            if (it->aIndex == atomIndex && it->bIndex != atomIndex && it->bIndex < atomStorage.size()) {
                ++atomStorage.valenceCount(it->bIndex);
            }
            if (it->bIndex == atomIndex && it->aIndex != atomIndex && it->aIndex < atomStorage.size()) {
                ++atomStorage.valenceCount(it->aIndex);
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

    atomStorage.removeAtom(atomIndex);
    sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
    return true;
}

void Simulation::addBond(size_t aIndex, size_t bIndex) {
    if (aIndex >= atomStorage.size() || bIndex >= atomStorage.size()) {
        return;
    }

    Bond::CreateBond(aIndex, bIndex, atomStorage);
}

void Simulation::clear() {
    atomStorage.clear();
    Bond::bonds_list.clear();
    sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
    neighborList.clear();
    sim_step = 0;
}
