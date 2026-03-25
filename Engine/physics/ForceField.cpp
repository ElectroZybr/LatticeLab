#include "ForceField.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "AtomData.h"
#include "Bond.h"
#include "../SimBox.h"
#include "../math/Consts.h"

ForceField::ForceField() : ljPairTable(buildLJPairTable()) {}

void ForceField::updateBoxCache(const SimBox& box) {
    wallMinX = 0.0f;
    wallMinY = 0.0f;
    wallMinZ = 0.0f;
    wallMaxX = static_cast<float>(box.end.x - box.start.x - 1.0);
    wallMaxY = static_cast<float>(box.end.y - box.start.y - 1.0);
    wallMaxZ = static_cast<float>(box.end.z - box.start.z - 1.0);
}

ForceField::LJPairTable ForceField::buildLJPairTable() {
    LJPairTable table{};
    constexpr int typeCount = static_cast<int>(table.size());

    for (int i = 0; i < typeCount; ++i) {
        const auto& pi = AtomData::getProps(static_cast<AtomData::Type>(i));
        const float a0i = static_cast<float>(pi.ljA0);
        const float epsi = static_cast<float>(pi.ljEps);

        for (int j = i; j < typeCount; ++j) {
            const auto& pj = AtomData::getProps(static_cast<AtomData::Type>(j));
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
    for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        ComputeForces(atoms, atomIndex, box);
    }

    for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end();) {
        Bond& bond = *it;
        if (bond.shouldBreak(atoms)) {
            Bond* currentBond = &bond;
            ++it;
            Bond::BreakBond(currentBond, atoms);
            continue;
        }

        bond.forceBond(atoms, dt);
        ++it;
    }

    std::vector<std::vector<std::size_t>> bondedNeighbours(atoms.size());
    for (const Bond& bond : Bond::bonds_list) {
        if (bond.aIndex >= atoms.size() || bond.bIndex >= atoms.size()) {
            continue;
        }

        bondedNeighbours[bond.aIndex].push_back(bond.bIndex);
        bondedNeighbours[bond.bIndex].push_back(bond.aIndex);
    }

    for (std::size_t atomIndex = 0; atomIndex < bondedNeighbours.size(); ++atomIndex) {
        const auto& neighbours = bondedNeighbours[atomIndex];
        if (neighbours.size() < 2) {
            continue;
        }

        for (std::size_t i = 0; i + 1 < neighbours.size(); ++i) {
            for (std::size_t j = i + 1; j < neighbours.size(); ++j) {
                Bond::angleForce(atoms, atomIndex, neighbours[i], neighbours[j]);
            }
        }
    }
}

void ForceField::softWalls(const AtomStorage& atoms, std::size_t atomIndex, float& forceX, float& forceY, float& forceZ) const {
    const float coordX = atoms.posX(atomIndex);
    const float coordY = atoms.posY(atomIndex);
    const float coordZ = atoms.posZ(atomIndex);

    applyWall(coordX, forceX, wallMinX, wallMaxX);
    applyWall(coordY, forceY, wallMinY, wallMaxY);
    applyWall(coordZ, forceZ, wallMinZ, wallMaxZ);
}

void ForceField::applyWall(float coord, float& force, float min, float max) {
    constexpr float k = 500.0f;
    constexpr float border = 2.0f;

    if (coord <= min || coord >= max) {
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

void ForceField::ComputeForces(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const {
    float posX = atoms.posX(atomIndex);
    float posY = atoms.posY(atomIndex);
    float posZ = atoms.posZ(atomIndex);
    float forceX = atoms.forceX(atomIndex);
    float forceY = atoms.forceY(atomIndex);
    float forceZ = atoms.forceZ(atomIndex);
    float potenE = atoms.energy(atomIndex);
    const LJPairRow& ljPairRow = ljPairTable[static_cast<std::size_t>(atoms.type(atomIndex))];

    softWalls(atoms, atomIndex, forceX, forceY, forceZ);
    applyGravityForce(forceX, forceY, forceZ);

    box.grid.forEachNeighborIndex(atoms.pos(atomIndex), [&](std::size_t neighbourIndex) {
        if (neighbourIndex >= atoms.size() || neighbourIndex <= atomIndex) {
            return;
        }
        pairNonBondedInteraction(atoms, neighbourIndex, ljPairRow, forceX, forceY, forceZ, posX, posY, posZ, potenE);
    });

    atoms.forceX(atomIndex) = forceX;
    atoms.forceY(atomIndex) = forceY;
    atoms.forceZ(atomIndex) = forceZ;
    atoms.energy(atomIndex) = potenE;
}

void ForceField::pairNonBondedInteraction(AtomStorage& atoms, std::size_t bIndex, const LJPairRow& ljPairRow,
                                          float& forceX, float& forceY, float& forceZ, float posX, float posY, float posZ, float& potenE) const {
    const float dx = atoms.posX(bIndex) - posX;
    const float dy = atoms.posY(bIndex) - posY;
    const float dz = atoms.posZ(bIndex) - posZ;
    const float d2 = dx * dx + dy * dy + dz * dz;
    if (d2 <= Consts::Epsilon) {
        return;
    }

    const LJParams& params = ljPairRow[static_cast<std::size_t>(atoms.type(bIndex))];

    const float invD2 = 1.0f / d2;
    const float invD6 = invD2 * invD2 * invD2;
    const float a2 = params.a0 * params.a0;
    const float a6 = a2 * a2 * a2;
    const float ratio6 = a6 * invD6;
    const float ratio12 = ratio6 * ratio6;

    const float forceScale = 24.0f * params.eps * (2.0f * ratio12 - ratio6) * invD2;
    const float pairForceX = dx * forceScale;
    const float pairForceY = dy * forceScale;
    const float pairForceZ = dz * forceScale;

    forceX -= pairForceX;
    forceY -= pairForceY;
    forceZ -= pairForceZ;

    atoms.forceX(bIndex) += pairForceX;
    atoms.forceY(bIndex) += pairForceY;
    atoms.forceZ(bIndex) += pairForceZ;

    const float potential = 4.0f * params.eps * (ratio12 - ratio6);
    potenE += 0.5f * potential;
    atoms.energy(bIndex) += 0.5f * potential;
}

void ForceField::applyGravityForce(float& forceX, float& forceY, float& forceZ) const {
    forceX += static_cast<float>(static_force.x);
    forceY += static_cast<float>(static_force.y);
    forceZ += static_cast<float>(static_force.z);
}
