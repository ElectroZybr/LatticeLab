#include <cmath>

#include "Simulation.h"
#include "io/SimulationStateIO.h"
#include "metrics/Profiler.h"
#include "physics/Bond.h"

Simulation::Simulation(SimBox& box)
    : sim_box(box), integrator() {
    atomStorage.reserve(250000);
    forceField.updateBoxCache(sim_box); // обновление кэша для параметров стен

    neighborList.setParams(5.f, 1.f); // параметры отсечки и скин для NL
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

void Simulation::update(float dt) {
    PROFILE_SCOPE("Simulation::update");
    integrator.step(atomStorage, sim_box, forceField, useNeighborList_ ? &neighborList : nullptr, dt);
    if (useNeighborList_ && neighborList.needsRebuild(atomStorage)) {
        neighborList.build(atomStorage, sim_box);
        neighborList.recordRebuild(sim_step);
    }
    ++sim_step;
}

void Simulation::setSizeBox(Vec3f newSize, int cellSize) {
    const bool resized = sim_box.setSizeBox(newSize, cellSize);
    if (resized) {
        forceField.updateBoxCache(sim_box);
        neighborList.clear();
    }
}

bool Simulation::createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed) {
    atomStorage.addAtom(start_coords, start_speed, type, fixed);
    sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
    return true;
}

bool Simulation::removeAtom(std::size_t atomIndex) {
    if (atomIndex >= atomStorage.size()) {
        return false;
    }

    const std::size_t lastIndex = atomStorage.size() - 1;

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

void Simulation::addBond(std::size_t aIndex, std::size_t bIndex) {
    if (aIndex >= atomStorage.size() || bIndex >= atomStorage.size()) {
        return;
    }

    Bond::CreateBond(aIndex, bIndex, atomStorage);
}

void Simulation::save(std::string_view path) const {
    SimulationStateIO::save(*this, path);
}

void Simulation::load(std::string_view path) {
    SimulationStateIO::load(*this, path);
}

void Simulation::clear() {
    atomStorage.clear();
    Bond::bonds_list.clear();
    sim_box.grid.rebuild(atomStorage.xDataSpan(), atomStorage.yDataSpan(), atomStorage.zDataSpan());
    neighborList.clear();
    sim_step = 0;
}
