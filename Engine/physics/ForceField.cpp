#include "ForceField.h"

#include <cmath>
#include <cstddef>

#include "Atom.h"
#include "../SimBox.h"
#include "../math/Consts.h"

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
            params.a0 = 0.5f * (a0i + a0j);
            params.eps = std::sqrt(epsi * epsj);

            table[i][j] = params;
            table[j][i] = params;
        }
    }

    return table;
}

void ForceField::compute(AtomStorage& atoms, SimBox& box, float dt) const {
    (void)dt;
    for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        ComputeForces(atoms, atomIndex, box);
    }
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

    applyWall(coordX, speedX, forceX, 0.0f, static_cast<float>(max.x));
    applyWall(coordY, speedY, forceY, 0.0f, static_cast<float>(max.y));
    applyWall(coordZ, speedZ, forceZ, 0.0f, static_cast<float>(max.z));

    atoms.posX(atomIndex) = coordX;
    atoms.posY(atomIndex) = coordY;
    atoms.posZ(atomIndex) = coordZ;

    atoms.velX(atomIndex) = speedX;
    atoms.velY(atomIndex) = speedY;
    atoms.velZ(atomIndex) = speedZ;

    atoms.forceX(atomIndex) = forceX;
    atoms.forceY(atomIndex) = forceY;
    atoms.forceZ(atomIndex) = forceZ;
}

void ForceField::applyWall(float& coord, float& speed, float& force, float min, float max) {
    constexpr float k = 500.0f;
    constexpr float border = 2.0f;

    // if (coord < min) {
    //     coord = min;
    //     if (speed < 0.0f) {
    //         speed = -speed * 0.8f;
    //     }
    //     return;
    // }

    // if (coord > max) {
    //     coord = max;
    //     if (speed > 0.0f) {
    //         speed = -speed * 0.8f;
    //     }
    //     return;
    // }

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

void ForceField::ComputeForces(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const {
    if (atomIndex >= atoms.size()) {
        return;
    }

    softWalls(atoms, atomIndex, box);
    applyGravityForce(atoms, atomIndex);

    box.grid.forEachNeighborIndex(atoms.pos(atomIndex), [&](std::size_t neighbourIndex) {
        if (neighbourIndex >= atoms.size() || neighbourIndex <= atomIndex) {
            return;
        }

        pairNonBondedInteraction(atoms, atomIndex, neighbourIndex);
    });
}

void ForceField::pairNonBondedInteraction(AtomStorage& atoms, std::size_t aIndex, std::size_t bIndex) const {
    const float dx = atoms.posX(bIndex) - atoms.posX(aIndex);
    const float dy = atoms.posY(bIndex) - atoms.posY(aIndex);
    const float dz = atoms.posZ(bIndex) - atoms.posZ(aIndex);
    const float d2 = dx * dx + dy * dy + dz * dz;
    if (d2 <= Consts::Epsilon) {
        return;
    }

    const LJParams params = ljPairTable[static_cast<std::size_t>(atoms.type(aIndex))]
                                       [static_cast<std::size_t>(atoms.type(bIndex))];

    const float invD2 = 1.0f / d2;
    const float invD6 = invD2 * invD2 * invD2;
    const float a2 = params.a0 * params.a0;
    const float a6 = a2 * a2 * a2;
    const float ratio6 = a6 * invD6;
    const float ratio12 = ratio6 * ratio6;

    const float forceScale = 24.0f * params.eps * (2.0f * ratio12 - ratio6) * invD2;
    const float forceX = dx * forceScale;
    const float forceY = dy * forceScale;
    const float forceZ = dz * forceScale;

    atoms.forceX(aIndex) -= forceX;
    atoms.forceY(aIndex) -= forceY;
    atoms.forceZ(aIndex) -= forceZ;

    atoms.forceX(bIndex) += forceX;
    atoms.forceY(bIndex) += forceY;
    atoms.forceZ(bIndex) += forceZ;

    const float potential = 4.0f * params.eps * (ratio12 - ratio6);
    atoms.energy(aIndex) += 0.5f * potential;
    atoms.energy(bIndex) += 0.5f * potential;
}

void ForceField::applyGravityForce(AtomStorage& atoms, std::size_t atomIndex) const {
    atoms.forceX(atomIndex) += static_cast<float>(static_force.x);
    atoms.forceY(atomIndex) += static_cast<float>(static_force.y);
    atoms.forceZ(atomIndex) += static_cast<float>(static_force.z);
}
