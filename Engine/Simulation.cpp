#include <fstream>
#include <cmath>
#include <iostream>

#include "Simulation.h"
#include "physics/Bond.h"

Simulation::Simulation(SimBox& box)
    :  sim_box(box), integrator()
{
    // СЂРµР·РµСЂРІРёСЂСѓРµРј РјРµСЃС‚Рѕ РїРѕРґ СЃРѕР·РґР°РЅРёРµ Р°С‚РѕРјРѕРІ
    atoms.reserve(50000);
    atomStorage.reserve(50000);
}

void Simulation::update(float dt) {
    integrator.step(atomStorage, atoms, sim_box, forceField, dt);
    ++sim_step;
}

void Simulation::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
    if (sim_box.setSizeBox(newStart, newEnd, cellSize)) {
        for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
            Atom& atom = atoms[atomIndex];
            const Vec3D pos = atomStorage.pos(atomIndex);
            const int cellX = sim_box.grid.worldToCellX(pos.x);
            const int cellY = sim_box.grid.worldToCellY(pos.y);
            const int cellZ = sim_box.grid.worldToCellZ(pos.z);
            sim_box.grid.insert(cellX, cellY, cellZ, &atom);
            sim_box.grid.insertIndex(cellX, cellY, cellZ, atomIndex);
        }
    }
}

void Simulation::createRandomAtoms(Atom::Type type, int quantity) {
    const double z_mid = (sim_box.end.z - sim_box.start.z) * 0.5;
    for (int i = 0; i < quantity; ++i) {
        for (int j = 0; j < 10; ++j) {
            double r_x = std::rand() % int(sim_box.end.x-sim_box.start.x-4);
            double r_y = std::rand() % int(sim_box.end.y-sim_box.start.y-4);
            Vec3D coords(r_x+2, r_y+2, z_mid);
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

Atom* Simulation::createAtom(Vec3D start_coords, Vec3D start_speed, Atom::Type type, bool fixed) {
    atomStorage.addAtom(start_coords, start_speed, type, fixed);
    atoms.emplace_back(start_coords, start_speed, type, fixed);
    Atom* atom = &atoms.back();
    const std::size_t atomIndex = atoms.size() - 1;
    const int cellX = sim_box.grid.worldToCellX(start_coords.x);
    const int cellY = sim_box.grid.worldToCellY(start_coords.y);
    const int cellZ = sim_box.grid.worldToCellZ(start_coords.z);
    sim_box.grid.insert(cellX, cellY, cellZ, atom);
    sim_box.grid.insertIndex(cellX, cellY, cellZ, atomIndex);
    return atom;
}

void Simulation::addBond(Atom* a1, Atom* a2) {
    if (!a1 || !a2 || atoms.empty()) {
        return;
    }

    const Atom* base = atoms.data();
    const Atom* end = base + atoms.size();
    if (a1 < base || a1 >= end || a2 < base || a2 >= end) {
        return;
    }

    const std::size_t aIndex = static_cast<std::size_t>(a1 - base);
    const std::size_t bIndex = static_cast<std::size_t>(a2 - base);
    Bond::CreateBond(aIndex, bIndex, atomStorage);
}

double Simulation::averageKineticEnegry() const {
    /* СЂР°СЃС‡РµС‚ СЃСЂРµРґРЅРµР№ РєРёРЅРµС‚РёС‡РµСЃРєРѕР№ СЌРЅРµСЂРіРёРё */
    if (atomStorage.empty()) {
        return 0.0;
    }

    double kineticEnergy = 0.0;
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        kineticEnergy += Atom::kineticEnergy(atomStorage.type(atomIndex), atomStorage.vel(atomIndex));
    }

    return kineticEnergy / static_cast<double>(atomStorage.size());
}

double Simulation::averagePotentialEnergy() const {
    /* СЂР°СЃС‡РµС‚ СЃСЂРµРґРЅРµР№ РїРѕС‚РµРЅС†РёР°Р»СЊРЅРѕР№ СЌРЅРµСЂРіРёРё */
    if (atomStorage.empty()) {
        return 0.0;
    }

    double potentialEnergy = 0.0;
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        potentialEnergy += atomStorage.energy(atomIndex);
    }

    return potentialEnergy / static_cast<double>(atomStorage.size());
}

double Simulation::fullAverageEnergy() const {
    /* СЂР°СЃС‡РµС‚ РїРѕР»РЅРѕР№ СЃСЂРµРґРЅРµР№ СЌРЅРµСЂРіРёРё */
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
    std::vector<int> bondCounts(atoms.size(), 0);
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

void Simulation::save(std::string_view path) const
{
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

    // РІСЂРµРјРµРЅРЅС‹Р№ Р±СѓС„РµСЂ С‡С‚РѕР±С‹ РЅРµ Р±С‹Р»Рѕ СЂРµР°Р»Р»РѕРєР°С†РёР№
    struct AtomData {
        Vec3D coords, speed;
        int type;
        float a0, eps;
        bool fixed;
    };
    std::vector<AtomData> buffer;

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
            AtomData d;
            file >> d.coords.x >> d.coords.y >> d.coords.z
                 >> d.speed.x  >> d.speed.y  >> d.speed.z
                 >> d.type >> d.a0 >> d.eps >> d.fixed;
            buffer.emplace_back(d);
        }
    }

    setSizeBox(boxStart, boxEnd, cellSize);

    atoms.reserve(buffer.size());
    for (const AtomData& d : buffer) {
        Atom* atom = createAtom(d.coords, d.speed, static_cast<Atom::Type>(d.type), d.fixed);
    }
}

void Simulation::clear() {
    atoms.clear();
    atomStorage.clear();
    Bond::bonds_list.clear();
    sim_step = 0;
}



