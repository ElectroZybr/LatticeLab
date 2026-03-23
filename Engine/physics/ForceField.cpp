#include "ForceField.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "Atom.h"
#include "Bond.h"
#include "../SimBox.h"
#include "../math/Consts.h"

namespace {
}

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

void ForceField::compute(std::vector<Atom>& atoms, SimBox& box, float dt) const {
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

void ForceField::compute(AtomStorage& atoms, SimBox& box, float dt) const {
    (void)dt;
    for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        ComputeForces(atoms, atomIndex, box);
    }
}

void ForceField::softWalls(Atom& atom, SimBox& box) const {
    const Vec3D max = box.end - box.start - Vec3D(1.0, 1.0, 1.0);

    float coordX = static_cast<float>(atom.coords.x);
    float coordY = static_cast<float>(atom.coords.y);
    float coordZ = static_cast<float>(atom.coords.z);

    float speedX = static_cast<float>(atom.speed.x);
    float speedY = static_cast<float>(atom.speed.y);
    float speedZ = static_cast<float>(atom.speed.z);

    float forceX = static_cast<float>(atom.force.x);
    float forceY = static_cast<float>(atom.force.y);
    float forceZ = static_cast<float>(atom.force.z);

    applyWall(coordX, speedX, forceX, 0.0f, static_cast<float>(max.x));
    applyWall(coordY, speedY, forceY, 0.0f, static_cast<float>(max.y));
    applyWall(coordZ, speedZ, forceZ, 0.0f, static_cast<float>(max.z));

    atom.coords.x = coordX;
    atom.coords.y = coordY;
    atom.coords.z = coordZ;

    atom.speed.x = speedX;
    atom.speed.y = speedY;
    atom.speed.z = speedZ;

    atom.force.x = forceX;
    atom.force.y = forceY;
    atom.force.z = forceZ;
}

void ForceField::softWalls(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const {
    const Vec3D max = box.end - box.start - Vec3D(1.0, 1.0, 1.0);

    float coordX = atoms.posX(atomIndex);
    float coordY = atoms.posY(atomIndex);
    float coordZ = atoms.posZ(atomIndex);

    float speedX = atoms.velX(atomIndex);
    float speedY = atoms.velY(atomIndex);
    float speedZ = atoms.velZ(atomIndex);

    float forceX = atoms.forceX(atomIndex);
    float forceY = atoms.forceY(atomIndex);
    float forceZ = atoms.forceZ(atomIndex);

    applyWall(coordX, speedX, forceX, 0.0, max.x);
    applyWall(coordY, speedY, forceY, 0.0, max.y);
    applyWall(coordZ, speedZ, forceZ, 0.0, max.z);

    atoms.posX(atomIndex) = static_cast<float>(coordX);
    atoms.posY(atomIndex) = static_cast<float>(coordY);
    atoms.posZ(atomIndex) = static_cast<float>(coordZ);

    atoms.velX(atomIndex) = static_cast<float>(speedX);
    atoms.velY(atomIndex) = static_cast<float>(speedY);
    atoms.velZ(atomIndex) = static_cast<float>(speedZ);

    atoms.forceX(atomIndex) = static_cast<float>(forceX);
    atoms.forceY(atomIndex) = static_cast<float>(forceY);
    atoms.forceZ(atomIndex) = static_cast<float>(forceZ);
}

void ForceField::applyWall(float& coord, float& speed, float& force, float min, float max) {
    constexpr float k = 500.0;
    constexpr float border = 2.0;

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
        const float penetration = (min + border) - coord;
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
        force += k * p4 * p2;
    } else if (coord > max - border) {
        const float penetration = coord - (max - border);
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
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

void ForceField::ComputeForces(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const {
    if (atomIndex >= atoms.size()) return;

    softWalls(atoms, atomIndex, box);
    applyGravityForce(atoms, atomIndex);
    box.grid.forEachNeighborIndex(atoms.pos(atomIndex), [&](std::size_t neighbourIndex) {
        if (neighbourIndex >= atoms.size() || neighbourIndex <= atomIndex) {
            return;
        }

        pairNonBondedInteraction(atoms, atomIndex, neighbourIndex);
    });
}

void ForceField::pairNonBondedInteraction(Atom& a, Atom& b) const {
    Vec3D delta = b.coords - a.coords;
    const float d2 = delta.sqrAbs();
    if (d2 <= Consts::Epsilon) return;

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

void ForceField::pairNonBondedInteraction(AtomStorage& atoms, std::size_t aIndex, std::size_t bIndex) const {
    const float dx = atoms.posX(bIndex) - atoms.posX(aIndex);
    const float dy = atoms.posY(bIndex) - atoms.posY(aIndex);
    const float dz = atoms.posZ(bIndex) - atoms.posZ(aIndex);
    const float d2 = dx * dx + dy * dy + dz * dz;
    if (d2 <= Consts::Epsilon) return;

    const LJParams params = ljPairTable[static_cast<std::size_t>(atoms.type(aIndex))]
                                       [static_cast<std::size_t>(atoms.type(bIndex))];

    const float inv_d2 = 1.f / d2;
    const float inv_d6 = inv_d2 * inv_d2 * inv_d2;
    const float a2 = params.a0 * params.a0;
    const float a6 = a2 * a2 * a2;
    const float ratio6 = a6 * inv_d6;
    const float ratio12 = ratio6 * ratio6;

    /* силы леннард джонса */
    const float forceScale = 24.0f * params.eps * (2.0f * ratio12 - ratio6) * inv_d2;
    const float forceX = dx * forceScale;
    const float forceY = dy * forceScale;
    const float forceZ = dz * forceScale;

    atoms.forceX(aIndex) -= forceX;
    atoms.forceY(aIndex) -= forceY;
    atoms.forceZ(aIndex) -= forceZ;

    atoms.forceX(bIndex) += forceX;
    atoms.forceY(bIndex) += forceY;
    atoms.forceZ(bIndex) += forceZ;

    /* потенциал леннард джонса */
    const float potential = 4.0f * params.eps * (ratio12 - ratio6);
    atoms.energy(aIndex) += 0.5f * potential;
    atoms.energy(bIndex) += 0.5f * potential;
}

void ForceField::applyGravityForce(Atom& atom) const {
    atom.force += static_force;
}

void ForceField::applyGravityForce(AtomStorage& atoms, std::size_t atomIndex) const {
    atoms.forceX(atomIndex) += static_cast<float>(static_force.x);
    atoms.forceY(atomIndex) += static_cast<float>(static_force.y);
    atoms.forceZ(atomIndex) += static_cast<float>(static_force.z);
}
