#include <fstream>
#include <cmath>
#include <iostream>

#include "Simulation.h"
#include "physics/Bond.h"

namespace {
float kineticEnergy(Atom::Type type, const Vec3D& speed) {
    return 0.5f * Atom::getProps(type).mass * speed.sqrAbs();
}
}

Simulation::Simulation(SimBox& box)
    : sim_box(box), integrator() {
    atomStorage.reserve(250000);
    forceField.updateBoxCache(sim_box); // обновление кэша для параметров стен
}

void Simulation::update(float dt) {
    integrator.step(atomStorage, sim_box, forceField, dt);
    ++sim_step;
}

void Simulation::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
    if (sim_box.setSizeBox(newStart, newEnd, cellSize)) {
        forceField.updateBoxCache(sim_box);
        for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
            const Vec3D pos = atomStorage.pos(atomIndex);
            const int cellX = sim_box.grid.worldToCellX(pos.x);
            const int cellY = sim_box.grid.worldToCellY(pos.y);
            const int cellZ = sim_box.grid.worldToCellZ(pos.z);
            sim_box.grid.insertIndex(cellX, cellY, cellZ, atomIndex);
        }
    }
}

void Simulation::createRandomAtoms(Atom::Type type, int quantity) {
    const double z_mid = (sim_box.end.z - sim_box.start.z) * 0.5;
    for (int i = 0; i < quantity; ++i) {
        for (int j = 0; j < 10; ++j) {
            double r_x = std::rand() % int(sim_box.end.x - sim_box.start.x - 4);
            double r_y = std::rand() % int(sim_box.end.y - sim_box.start.y - 4);
            Vec3D coords(r_x + 2, r_y + 2, z_mid);
            if (!checkNeighbor(coords, 4)) {
                createAtom(coords, Vec3D::Random() * 5.0, type);
                break;
            }
        }
    }
}

bool Simulation::checkNeighbor(Vec3D coords, float delta) {
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

bool Simulation::createAtom(Vec3D start_coords, Vec3D start_speed, Atom::Type type, bool fixed) {
    atomStorage.addAtom(start_coords, start_speed, type, fixed);
    atoms.emplace_back(start_coords, start_speed, type, fixed);
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

    if (atomIndex < atoms.size()) {
        const std::size_t lastAtomIndex = atoms.size() - 1;
        if (atomIndex != lastAtomIndex) {
            std::swap(atoms[atomIndex], atoms[lastAtomIndex]);
        }
        atoms.pop_back();
    }

    atomStorage.removeAtom(atomIndex);

    sim_box.grid.resize(sim_box.grid.sizeX, sim_box.grid.sizeY, sim_box.grid.sizeZ, sim_box.grid.cellSize);
    for (std::size_t index = 0; index < atomStorage.size(); ++index) {
        const Vec3D pos = atomStorage.pos(index);
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

float Simulation::averageKineticEnegry() const {
    if (atomStorage.empty()) {
        return 0.0;
    }

    double kineticEnergy = 0.0;
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        kineticEnergy += ::kineticEnergy(atomStorage.type(atomIndex), atomStorage.vel(atomIndex));
    }

    return kineticEnergy / static_cast<double>(atomStorage.size());
}

float Simulation::averagePotentialEnergy() const {
    if (atomStorage.empty()) {
        return 0.0;
    }

    double potentialEnergy = 0.0;
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        potentialEnergy += atomStorage.energy(atomIndex);
    }

    return potentialEnergy / static_cast<double>(atomStorage.size());
}

float Simulation::fullAverageEnergy() const {
    return averageKineticEnegry() + averagePotentialEnergy();
}

void Simulation::logAtomPos() const {
    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const Vec3D pos = atomStorage.pos(i);
        std::cout << "<Pos> Atom (" << i
                  << ") X " << pos.x
                  << " | Y " << pos.y
                  << " | Z " << pos.z
                  << std::endl;
    }
}

void Simulation::logBondList() const {
    std::vector<int> bondCounts(atomStorage.size(), 0);
    for (const Bond& bond : Bond::bonds_list) {
        if (bond.aIndex < bondCounts.size()) {
            ++bondCounts[bond.aIndex];
        }
        if (bond.bIndex < bondCounts.size()) {
            ++bondCounts[bond.bIndex];
        }
    }

    for (int count : bondCounts) {
        if (count > 0) {
            std::cout << count << std::endl;
        }
    }
}

void Simulation::save(std::string_view path) const {
    std::ofstream file(path.data());
    if (!file.is_open()) return;

    file << "box "
         << sim_box.start.x << " " << sim_box.start.y << " " << sim_box.start.z << " "
         << sim_box.end.x   << " " << sim_box.end.y   << " " << sim_box.end.z   << "\n";

    file << "step " << sim_step << "\n";

    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        const Vec3D pos = atomStorage.pos(atomIndex);
        const Vec3D vel = atomStorage.vel(atomIndex);
        file << "atom "
             << pos.x << " " << pos.y << " " << pos.z << " "
             << vel.x << " " << vel.y << " " << vel.z << " "
             << static_cast<int>(atomStorage.type(atomIndex)) << " "
             << atomStorage.isAtomFixed(atomIndex) << "\n";
    }
}

void Simulation::load(std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) return;

    clear();

    struct LoadedAtomData {
        Vec3D coords, speed;
        int type;
        float a0, eps;
        bool fixed;
    };
    std::vector<LoadedAtomData> buffer;

    Vec3D boxStart, boxEnd;
    int cellSize = -1;

    std::string tag;
    while (file >> tag) {
        if (tag == "box") {
            file >> boxStart.x >> boxStart.y >> boxStart.z
                 >> boxEnd.x   >> boxEnd.y   >> boxEnd.z;
        }
        else if (tag == "step") {
            file >> sim_step;
        }
        else if (tag == "atom") {
            LoadedAtomData d;
            file >> d.coords.x >> d.coords.y >> d.coords.z
                 >> d.speed.x  >> d.speed.y  >> d.speed.z
                 >> d.type >> d.a0 >> d.eps >> d.fixed;
            buffer.emplace_back(d);
        }
    }

    setSizeBox(boxStart, boxEnd, cellSize);

    for (const LoadedAtomData& d : buffer) {
        createAtom(d.coords, d.speed, static_cast<Atom::Type>(d.type), d.fixed);
    }
}

void Simulation::clear() {
    atoms.clear();
    atomStorage.clear();
    Bond::bonds_list.clear();
    sim_box.grid.resize(sim_box.grid.sizeX, sim_box.grid.sizeY, sim_box.grid.sizeZ, sim_box.grid.cellSize);
    sim_step = 0;
}

