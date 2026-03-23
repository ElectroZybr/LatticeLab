#include "ForceField.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "Atom.h"
#include "Bond.h"
#include "../SimBox.h"

ForceField::ForceField() : ljPairTable(buildLJPairTable()) {}

ForceField::LJPairTable ForceField::buildLJPairTable() {
    LJPairTable table{};
    constexpr int typeCount = static_cast<int>(table.size());

    for (int i = 0; i < typeCount; ++i) {
        const auto& pi = Atom::getProps(static_cast<Atom::Type>(i));
        const float a0i = static_cast<float>(pi.ljA0);
        const float epsi = static_cast<float>(pi.ljEps);

        for (int j = i; j < typeCount; ++j) {
            const auto& pj = Atom::getProps(static_cast<Atom::Type>(j));
            const float a0j = static_cast<float>(pj.ljA0);
            const float epsj = static_cast<float>(pj.ljEps);

            LJParams params{};
            params.a0 = 0.5f * (a0i + a0j);       // усреднение для параметра a0
            params.eps = std::sqrt(epsi * epsj);  // геометрическое среднее для параметра eps

            table[i][j] = params;
            table[j][i] = params;
        }
    }

    return table;
}

void ForceField::compute(std::vector<Atom>& atoms, SimBox& box, double dt) const {
    for (Atom& atom : atoms) {
        ComputeForces(atom, box);
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

void ForceField::softWalls(Atom& atom, SimBox& box) const {
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

void ForceField::ComputeForces(Atom& atom, SimBox& box) const {
    softWalls(atom, box);
    applyGravityForce(atom);

    /* перебор соседей атома */
    box.grid.forEachNeighbor(atom.coords, [&](Atom* neighbour) {
        if (neighbour <= &atom) return;

        // Vec3D delta = atom.coords - neighbour->coords;
        // float distance = sqrt(delta.dot(delta));
        
        const bool bonded = std::find(atom.bonds.begin(), atom.bonds.end(), neighbour) != atom.bonds.end();

        if (atom.getProps().maxValence - atom.valence >= 2) {
            Bond::angleForce(&atom, atom.bonds[0], atom.bonds[1]);
        }

        if (!bonded) {
            // if (distance < 1.3 * 3 && atom.valence > 0 && neighbour->valence > 0) {
            //     Bond::CreateBond(&atom, neighbour);
            // }
            pairNonBondedInteraction(atom, *neighbour);
        }
    });
}

void ForceField::pairNonBondedInteraction(Atom& a, Atom& b) const {
    Vec3D delta = b.coords - a.coords;
    const float d2 = delta.sqrAbs();
    if (d2 <= 1e-18f) return;

    LJParams params = ljPairTable[static_cast<std::size_t>(a.type)][static_cast<std::size_t>(b.type)];

    const float inv_d2 = 1.f / d2;
    const float inv_d6 = inv_d2 * inv_d2 * inv_d2;
    const float a2 = params.a0 * params.a0;
    const float a6 = a2 * a2 * a2;
    const float ratio6 = a6 * inv_d6;
    const float ratio12 = ratio6 * ratio6;

    /* силы леннард джонса */
    const Vec3D force = delta * 24.0f * params.eps * (2.0f * ratio12 - ratio6) * inv_d2;
    a.force -= force;
    b.force += force;

    /* потенциал леннард джонса */
    // TODO: убрать расчет на каждой итерации
    const float potential = 4.0f * params.eps * (ratio12 - ratio6);
    a.potential_energy += 0.5f * potential;
    b.potential_energy += 0.5f * potential;
}

void ForceField::applyGravityForce(Atom& atom) const {
    atom.force += static_force;
}