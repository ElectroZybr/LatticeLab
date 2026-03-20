#include "ForceField.h"

#include <algorithm>

#include "Atom.h"
#include "Bond.h"
#include "../SimBox.h"

void ForceField::compute(std::vector<Atom>& atoms, SimBox& box, double dt) const {
    for (Atom& atom : atoms) {
        ComputeForces(atom, box, dt);
    }

    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        if (it->shouldBreak()) {
            it->detach();
            it = Bond::bonds_list.erase(it);
        } else {
            ++it;
        }
    }

    for (Bond& bond : Bond::bonds_list) {
        bond.forceBond(dt);
    }
}

void ForceField::softWalls(Atom& atom, SimBox& box, double /*dt*/) const {
    const Vec3D max = box.end - box.start - Vec3D(1.0, 1.0, 1.0);
    applyWall(atom.coords.x, atom.speed.x, atom.force.x, 0.0, max.x);
    applyWall(atom.coords.y, atom.speed.y, atom.force.y, 0.0, max.y);
    applyWall(atom.coords.z, atom.speed.z, atom.force.z, 0.0, max.z);
}

void ForceField::applyWall(double& coord, double& speed, double& force, double min, double max) {
    constexpr double k = 500.0;
    constexpr double border = 2.0;

    if (coord < min) {
        coord = min;
        if (speed < 0.0) speed = -speed * 0.8;
        return;
    }

    if (coord > max) {
        coord = max;
        if (speed > 0.0) speed = -speed * 0.8;
        return;
    }

    if (coord < min + border) {
        const double penetration = (min + border) - coord;
        const double p2 = penetration * penetration;
        const double p4 = p2 * p2;
        force += k * p4 * p2;
    } else if (coord > max - border) {
        const double penetration = coord - (max - border);
        const double p2 = penetration * penetration;
        const double p4 = p2 * p2;
        force -= k * p4 * p2;
    }
}

void ForceField::ComputeForces(Atom& atom, SimBox& box, double dt) const {
    softWalls(atom, box, dt);

    SpatialGrid& grid = box.grid;
    const int curr_x = grid.worldToCellX(atom.coords.x);
    const int curr_y = grid.worldToCellY(atom.coords.y);
    const int curr_z = grid.worldToCellZ(atom.coords.z);
    constexpr int range = 1;
    // проверка взаимодействий с соседними атомами
    for (int i = -range; i <= range; ++i) {
        for (int j = -range; j <= range; ++j) {
            for (int k = -range; k <= range; ++k) {
                if (auto* cell = grid.at(curr_x - i, curr_y - j, curr_z - k)) {
                    for (Atom* other : *cell) {
                        // хитрожопая проверка
                        if (other <= &atom) continue;

                        // Vec3D delta = coords - other->coords;
                        // float distance = sqrt(delta.dot(delta));
                        
                        const bool bonded = std::find(atom.bonds.begin(), atom.bonds.end(), other) != atom.bonds.end();

                        if (atom.getProps().maxValence - atom.valence >= 2) {
                            Bond::angleForce(&atom, atom.bonds[0], atom.bonds[1]);
                        }

                        if (!bonded) {
                            // if (distance < 1.3 * r0 && valence > 0 && other->valence > 0) {
                            //     Bond::CreateBond(this, other);
                            // }
                            this->pairNonBondedInteraction(atom, *other);
                        }
                    }
                }
            }
        }
    }
}

void ForceField::pairNonBondedInteraction(Atom& a, Atom& b) const {
    /* расчет парных взаимодействий */
    Vec3D delta = b.coords - a.coords;
    const float d = delta.length();
    if (d <= 1e-9f) return;         // расстояние между атомами
    const Vec3D hat = delta / d;    // единичный вектор от a к b

    const float inv_d2  = 1.f / (d * d);            // 1/d^2
    const float inv_d6  = inv_d2 * inv_d2 * inv_d2; // 1/d^6
    const float a2      = a.a0 * a.a0;              // a^2
    const float a6      = a2 * a2 * a2;             // a^6
    const float ratio6  = a6 * inv_d6;              // (a/d)^6
    const float ratio12 = ratio6 * ratio6;          // (a/d)^12

    /* силы леннард джонса */
    const Vec3D force = hat * 24.0f * a.eps * (2.0f * ratio12 - ratio6) / d;
    a.force -= force;
    b.force += force;

    /* потенциал леннард джонса */
    // TODO: убрать расчет на каждой итерации
    const float potential = 4.0f * a.eps * (ratio12 - ratio6);
    a.potential_energy += 0.5f * potential;
    b.potential_energy += 0.5f * potential;
}
