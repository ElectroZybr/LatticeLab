#include <fstream>
#include <cmath>
#include <iostream>

#include "Simulation.h"

Simulation::Simulation(SimBox& box)
    :  sim_box(box), integrator()
{
    // резервируем место под создание атомов
    atoms.reserve(50000);
}

void Simulation::update(float dt) {
    integrator.step(atoms, sim_box, forceField, dt);
    ++sim_step;
}

void Simulation::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
    if (sim_box.setSizeBox(newStart, newEnd, cellSize)) {
        for (Atom& atom : atoms) {
            const int cellX = sim_box.grid.worldToCellX(atom.coords.x);
            const int cellY = sim_box.grid.worldToCellY(atom.coords.y);
            const int cellZ = sim_box.grid.worldToCellZ(atom.coords.z);
            sim_box.grid.insert(cellX, cellY, cellZ, &atom);
        }
    }
}

void Simulation::createRandomAtoms(int type, int quantity) {
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
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                if (auto cell = sim_box.grid.at(curr_x - i, curr_y - j, curr_z - k)) {
                    for (Atom* other : *cell) {
                        if ((coords - other->coords).sqrAbs() < delta*delta) return true;
                    }
                }
            }
        }
    }
    return false;
}

Atom* Simulation::createAtom(Vec3D start_coords, Vec3D start_speed, int type, bool fixed) {
    atoms.emplace_back(start_coords, start_speed, type, fixed);
    Atom* atom = &atoms.back();
    const int cellX = sim_box.grid.worldToCellX(atom->coords.x);
    const int cellY = sim_box.grid.worldToCellY(atom->coords.y);
    const int cellZ = sim_box.grid.worldToCellZ(atom->coords.z);
    sim_box.grid.insert(cellX, cellY, cellZ, atom);
    return atom;
}

void Simulation::addBond(Atom* a1, Atom* a2) {
    // FIXME Здесь возможен баг, что если вектор атомов переаллоцирован, то указатели станут не валидными
    Bond::CreateBond(a1, a2);
}

double Simulation::averageKineticEnegry() const {
    /* расчет средней кинетической энергии */
    if (atoms.empty()) {
        return 0.0;
    }

    double kineticEnergy = 0.0;
    for (const Atom& atom : atoms) {
        kineticEnergy += atom.kineticEnergy();
    }

    return kineticEnergy / static_cast<double>(atoms.size());
}

double Simulation::averagePotentialEnergy() const {
    /* расчет средней потенциальной энергии */
    if (atoms.empty()) {
        return 0.0;
    }

    double potentialEnergy = 0.0;
    for (const Atom& atom : atoms) {
        potentialEnergy += atom.potential_energy;
    }

    return potentialEnergy / static_cast<double>(atoms.size());
}

double Simulation::fullAverageEnergy() const {
    /* расчет полной средней энергии */
    return averageKineticEnegry() + averagePotentialEnergy();
}

void Simulation::logAtomPos() const {
    int i = 0;
    for (const Atom& atom : atoms) {
        std::cout << "<Pos> Atom (" << i
                  << ") X " << atom.coords.x
                  << " | Y " << atom.coords.y
                  << " | Z " << atom.coords.z
                  << std::endl;
        i++;
    }
}

void Simulation::logBondList() const {
    for (const Atom& atom : atoms) {
        if (atom.bonds.size() > 0) {
            std::cout << atom.bonds.size() << std::endl;
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

    for (const Atom& atom : atoms) {
        file << "atom "
             << atom.coords.x << " " << atom.coords.y << " " << atom.coords.z << " "
             << atom.speed.x  << " " << atom.speed.y  << " " << atom.speed.z  << " "
             << atom.type     << " "
             << atom.isFixed  << "\n";
    }
}

void Simulation::load(std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) return;

    clear();

    // временный буфер чтобы не было реаллокаций
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
        Atom* atom = createAtom(d.coords, d.speed, d.type, d.fixed);
    }
}

void Simulation::clear() {
    atoms.clear();
    Bond::bonds_list.clear();
    sim_step = 0;
}
