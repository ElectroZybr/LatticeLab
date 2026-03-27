#include <cmath>

#include "Simulation.h"
#include "io/SimulationStateIO.h"
#include "metrics/EnergyMetrics.h"
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
        neighborListMetrics_.onDisable();
    }
}

void Simulation::update(float dt) {
    if (useNeighborList_ && neighborList.needsRebuild(atomStorage)) {
        neighborList.build(atomStorage, sim_box);
        neighborListMetrics_.onRebuild(sim_step, neighborList.buildCounter());
    }
    integrator.step(atomStorage, sim_box, forceField, useNeighborList_ ? &neighborList : nullptr, dt);
    ++sim_step;
}

void Simulation::setSizeBox(Vec3f newStart, Vec3f newEnd, int cellSize) {
    if (sim_box.setSizeBox(newStart, newEnd, cellSize)) {
        forceField.updateBoxCache(sim_box);
        for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
            const Vec3f pos = atomStorage.pos(atomIndex);
            const int cellX = sim_box.grid.worldToCellX(pos.x);
            const int cellY = sim_box.grid.worldToCellY(pos.y);
            const int cellZ = sim_box.grid.worldToCellZ(pos.z);
            sim_box.grid.insertIndex(cellX, cellY, cellZ, atomIndex);
        }
    }
}

void Simulation::createRandomAtoms(AtomData::Type type, int quantity) {
    const double z_mid = (sim_box.end.z - sim_box.start.z) * 0.5;
    for (int i = 0; i < quantity; ++i) {
        for (int j = 0; j < 10; ++j) {
            double r_x = std::rand() % int(sim_box.end.x - sim_box.start.x - 4);
            double r_y = std::rand() % int(sim_box.end.y - sim_box.start.y - 4);
            Vec3f coords(r_x + 2, r_y + 2, z_mid);
            if (!checkNeighbor(coords, 4)) {
                createAtom(coords, Vec3f::Random() * 5.0, type);
                break;
            }
        }
    }
}

bool Simulation::checkNeighbor(Vec3f coords, float delta) {
    int curr_x = sim_box.grid.worldToCellX(coords.x);
    int curr_y = sim_box.grid.worldToCellY(coords.y);
    int curr_z = sim_box.grid.worldToCellZ(coords.z);
    const float deltaSqr = delta * delta;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                if (auto cell = sim_box.grid.atIndex(curr_x - i, curr_y - j, curr_z - k)) {
                    for (std::size_t atomIndex : *cell) {
                        if (atomIndex >= atomStorage.size()) {
                            continue;
                        }

                        if ((coords - atomStorage.pos(atomIndex)).sqrAbs() < deltaSqr) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool Simulation::createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed) {
    atomStorage.addAtom(start_coords, start_speed, type, fixed);
    const std::size_t atomIndex = atomStorage.size() - 1;
    const int cellX = sim_box.grid.worldToCellX(start_coords.x);
    const int cellY = sim_box.grid.worldToCellY(start_coords.y);
    const int cellZ = sim_box.grid.worldToCellZ(start_coords.z);
    sim_box.grid.insertIndex(cellX, cellY, cellZ, atomIndex);
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

    sim_box.grid.clear();
    for (std::size_t index = 0; index < atomStorage.size(); ++index) {
        const Vec3f pos = atomStorage.pos(index);
        const int cellX = sim_box.grid.worldToCellX(pos.x);
        const int cellY = sim_box.grid.worldToCellY(pos.y);
        const int cellZ = sim_box.grid.worldToCellZ(pos.z);
        sim_box.grid.insertIndex(cellX, cellY, cellZ, index);
    }

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
    sim_box.grid.clear();
    neighborList.clear();
    sim_step = 0;
    integrator.resetMetrics();
    neighborListMetrics_.reset();
}

float Simulation::averageKineticEnegry() const {
    return EnergyMetrics::averageKineticEnergy(atomStorage);
}

float Simulation::averagePotentialEnergy() const {
    return EnergyMetrics::averagePotentialEnergy(atomStorage);
}

float Simulation::fullAverageEnergy() const {
    return EnergyMetrics::fullAverageEnergy(atomStorage);
}

float Simulation::averageStepsPerNeighborListRebuild() const {
    return neighborListMetrics_.averageStepsBetweenRebuilds();
}

float Simulation::recentAverageStepsPerNeighborListRebuild() const {
    return neighborListMetrics_.recentAverageStepsBetweenRebuilds();
}

int Simulation::stepsSinceNeighborListRebuild() const {
    return neighborListMetrics_.stepsSinceLastRebuild(sim_step);
}

float Simulation::lastNeighborListRebuildTimeMs() const {
    return neighborListMetrics_.lastRebuildTimeMs();
}

float Simulation::averageNeighborListRebuildTimeMs() const {
    return neighborListMetrics_.averageRebuildTimeMs();
}

float Simulation::maxNeighborListRebuildTimeMs() const {
    return neighborListMetrics_.maxRebuildTimeMs();
}
