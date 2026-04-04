#include "ForceField.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "AtomData.h"
#include "Bond.h"
#include "Engine/metrics/Profiler.h"
#include "Engine/Consts.h"
#include "Engine/NeighborSearch/NeighborList.h"

ForceField::ForceField() : ljPairTable(buildLJPairTable()) {}

void ForceField::syncWalls(const SimBox& box) {
    wallMaxX = box.size.x - 1.0f;
    wallMaxY = box.size.y - 1.0f;
    wallMaxZ = box.size.z - 1.0f;
}

ForceField::LJPairTable ForceField::buildLJPairTable() {
    /* построение таблицы LJ-параметров */
    LJPairTable table{};
    constexpr int typeCount = static_cast<int>(table.size());

    for (int i = 0; i < typeCount; ++i) {
        const auto& pi = AtomData::getProps(static_cast<AtomData::Type>(i));
        const float a0i = pi.ljA0;
        const float epsi = pi.ljEps;

        for (int j = i; j < typeCount; ++j) {
            const auto& pj = AtomData::getProps(static_cast<AtomData::Type>(j));
            const float a0j = pj.ljA0;
            const float epsj = pj.ljEps;

            LJParams params{};
            const float a0 = 0.5f * (a0i + a0j);
            const float eps = std::sqrt(epsi * epsj);
            const float a2 = a0 * a0;
            const float a6 = a2 * a2 * a2;
            const float a12 = a6 * a6;

            params.forceC6 = 24.0f * eps * a6;
            params.forceC12 = 48.0f * eps * a12;
            params.potentialC6 = 4.0f * eps * a6;
            params.potentialC12 = 4.0f * eps * a12;

            table[i][j] = params;
            table[j][i] = params;
        }
    }

    return table;
}

void ForceField::compute(AtomStorage& atoms, Bond::List& bonds, SimBox& box, NeighborList& neighborList, bool allowBondFormation, float dt) const {
    PROFILE_SCOPE("ForceField::compute");

    // расчет нековалентных сил для каждого атома
    ComputeForces(atoms, box, neighborList);

    // проверка образования и разрыва связей, а также расчет сил
    std::erase_if(bonds, [&](Bond& bond) {
        if (bond.shouldBreak(atoms)) {
            bond.detach(atoms);
            return true;
        }
        return false;
    });

    if (allowBondFormation) {
        FormBonds(atoms, bonds, box, neighborList);
    }

    for (Bond& bond : bonds) {
        bond.forceBond(atoms, dt);
    }

    if (bonds.size() < 2) {
        return;
    }

    std::vector<uint16_t> degree(atoms.size(), 0);
    for (const Bond& bond : bonds) {
        if (bond.aIndex < atoms.size() && bond.bIndex < atoms.size()) {
            ++degree[bond.aIndex];
            ++degree[bond.bIndex];
        }
    }

    std::vector<std::vector<size_t>> bondedNeighbours(atoms.size());
    for (size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        if (degree[atomIndex] > 0) {
            bondedNeighbours[atomIndex].reserve(degree[atomIndex]);
        }
    }

    for (const Bond& bond : bonds) {
        if (bond.aIndex < atoms.size() && bond.bIndex < atoms.size()) {
            bondedNeighbours[bond.aIndex].emplace_back(bond.bIndex);
            bondedNeighbours[bond.bIndex].emplace_back(bond.aIndex);
        }
    }

    for (size_t atomIndex = 0; atomIndex < bondedNeighbours.size(); ++atomIndex) {
        const auto& neighbours = bondedNeighbours[atomIndex];
        if (neighbours.size() < 2) {
            continue;
        }

        for (size_t i = 0; i + 1 < neighbours.size(); ++i) {
            for (size_t j = i + 1; j < neighbours.size(); ++j) {
                Bond::angleForce(atoms, atomIndex, neighbours[i], neighbours[j]);
            }
        }
    }
}

void ForceField::softWalls(const AtomStorage& atoms, float coordX, float coordY, float coordZ, float& forceX, float& forceY, float& forceZ) const {
    applyWall(coordX, forceX, wallMaxX);
    applyWall(coordY, forceY, wallMaxY);
    applyWall(coordZ, forceZ, wallMaxZ);
}

void ForceField::applyWall(float coord, float& force, float max) {
    constexpr float k = 500.0f;
    constexpr float border = 2.0f;

    if (coord <= 0 || coord >= max) {
        return;
    }

    if (coord < border) {
        const float penetration = border - coord;
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
        force += k * p4 * p2;
    }
    else if (coord > max - border) {
        const float penetration = coord - (max - border);
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
        force -= k * p4 * p2;
    }
}

void ForceField::ComputeForces(AtomStorage& atoms, SimBox& box, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::ComputeForces(NL)");
    for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
        // загружаем данные текущего атома из AtomStorage
        float posX = atoms.posX(atomIndex);
        float posY = atoms.posY(atomIndex);
        float posZ = atoms.posZ(atomIndex);
        float forceX = atoms.forceX(atomIndex);
        float forceY = atoms.forceY(atomIndex);
        float forceZ = atoms.forceZ(atomIndex);
        float potenE = atoms.energy(atomIndex);
        // выбираем строку таблицы LJ для данного типа атома
        const LJPairRow& ljPairRow = ljPairTable[static_cast<size_t>(atoms.type(atomIndex))];

        // мягкие стены
        softWalls(atoms, posX, posY, posZ, forceX, forceY, forceZ);
        // постоянная сила
        applyGravityForce(forceX, forceY, forceZ);

        // используем список соседей
        const auto& off = neighborList.offsets();
        const auto& nei = neighborList.neighbors();
        if (atomIndex + 1 >= off.size()) {
            atoms.forceX(atomIndex) = forceX;
            atoms.forceY(atomIndex) = forceY;
            atoms.forceZ(atomIndex) = forceZ;
            atoms.energy(atomIndex) = potenE;
            continue;
        }
        const uint32_t begin = off[atomIndex];
        const uint32_t end = off[atomIndex + 1];
        if (begin > end || static_cast<size_t>(end) > nei.size()) {
            atoms.forceX(atomIndex) = forceX;
            atoms.forceY(atomIndex) = forceY;
            atoms.forceZ(atomIndex) = forceZ;
            atoms.energy(atomIndex) = potenE;
            continue;
        }
        for (uint32_t p = begin; p < end; ++p) {
            const uint32_t neighbourIndex = nei[p];
            pairNonBondedInteraction(atoms, neighbourIndex, ljPairRow, forceX, forceY, forceZ, posX, posY, posZ, potenE);
        }

        // записываем обратно в AtomStorage
        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
        atoms.energy(atomIndex) = potenE;
    }
}

void ForceField::FormBonds(AtomStorage& atoms, Bond::List& bonds, SimBox& box, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::FormBonds(NL)");
    const uint32_t atomCount = static_cast<uint32_t>(atoms.size());
    if (atomCount < 2) {
        return;
    }

    const auto& off = neighborList.offsets();
    const auto& nei = neighborList.neighbors();
    for (uint32_t atomIndex = 0; atomIndex < atomCount; ++atomIndex) {
        if (atomIndex + 1 >= off.size()) {
            continue;
        }
        const uint32_t begin = off[atomIndex];
        const uint32_t end = off[atomIndex + 1];
        if (begin > end || static_cast<size_t>(end) > nei.size()) {
            continue;
        }
        for (uint32_t p = begin; p < end; ++p) {
            tryCreateBond(atoms, bonds, atomIndex, nei[p]);
        }
    }
}

void ForceField::tryCreateBond(AtomStorage& atoms, Bond::List& bonds, uint32_t aIndex, uint32_t bIndex) const {
    Bond::ensureInitialized();

    if (aIndex >= atoms.size() || bIndex >= atoms.size() || aIndex == bIndex) {
        return;
    }

    const BondParams& bondParams = Bond::bond_default_props.get(atoms.type(aIndex), atoms.type(bIndex));
    if (bondParams.r0 <= 0.0f || bondParams.a <= 0.0f || bondParams.De <= 0.0f) {
        return;
    }

    const float dx = atoms.posX(bIndex) - atoms.posX(aIndex);
    const float dy = atoms.posY(bIndex) - atoms.posY(aIndex);
    const float dz = atoms.posZ(bIndex) - atoms.posZ(aIndex);
    const float distanceSqr = dx * dx + dy * dy + dz * dz;

    const float formationDistance = std::max(2.5f, bondParams.r0 * 1.35f);
    if (distanceSqr > formationDistance * formationDistance) {
        return;
    }

    Bond::CreateBond(bonds, aIndex, bIndex, atoms);
}

void ForceField::pairNonBondedInteraction(AtomStorage& atoms, uint32_t bIndex, const LJPairRow& ljPairRow,
                                          float& forceX, float& forceY, float& forceZ, float posX, float posY, float posZ, float& potenE) const {
    // расчет вектора между атомами
    const float dx = atoms.posX(bIndex) - posX;
    const float dy = atoms.posY(bIndex) - posY;
    const float dz = atoms.posZ(bIndex) - posZ;
    const float d2 = dx * dx + dy * dy + dz * dz;
    if (d2 <= Consts::Epsilon) {
        return;
    }

    // параметры LJ для данной пары атомов
    const LJParams& params = ljPairRow[static_cast<size_t>(atoms.type(bIndex))];

    // предварительные расчеты для LJ потенциала
    const float invD2 = 1.0f / d2;
    const float invD6 = invD2 * invD2 * invD2;
    const float invD12 = invD6 * invD6;

    // расчет сил LJ
    const float forceScale = (params.forceC12 * invD12 - params.forceC6 * invD6) * invD2;
    const float pairForceX = dx * forceScale;
    const float pairForceY = dy * forceScale;
    const float pairForceZ = dz * forceScale;

    // добавляем силу в буфер первого атома
    forceX -= pairForceX;
    forceY -= pairForceY;
    forceZ -= pairForceZ;

    // применяем силу второму атому
    atoms.forceX(bIndex) += pairForceX;
    atoms.forceY(bIndex) += pairForceY;
    atoms.forceZ(bIndex) += pairForceZ;

    // расчет потенциальной энергии
    const float potential = params.potentialC12 * invD12 - params.potentialC6 * invD6;
    potenE += 0.5f * potential;
    atoms.energy(bIndex) += 0.5f * potential;
}

void ForceField::applyGravityForce(float& forceX, float& forceY, float& forceZ) const {
    forceX += static_force.x;
    forceY += static_force.y;
    forceZ += static_force.z;
}
