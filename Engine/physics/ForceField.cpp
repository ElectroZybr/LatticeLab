#include "ForceField.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "AtomData.h"
#include "Bond.h"
#include "../SimBox.h"
#include "../math/Consts.h"
#include "../NeighborSearch/NeighborList.h"

ForceField::ForceField() : ljPairTable(buildLJPairTable()) {}

void ForceField::updateBoxCache(const SimBox& box) {
    /* кеш границ стен */
    wallMinX = 0.0f;
    wallMinY = 0.0f;
    wallMinZ = 0.0f;
    wallMaxX = static_cast<float>(box.end.x - box.start.x - 1.0);
    wallMaxY = static_cast<float>(box.end.y - box.start.y - 1.0);
    wallMaxZ = static_cast<float>(box.end.z - box.start.z - 1.0);
}

ForceField::LJPairTable ForceField::buildLJPairTable() {
    /* построение таблицы LJ-параметров */
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

void ForceField::compute(AtomStorage& atoms, SimBox& box, NeighborList* neighborList, float dt) const {
    if (!neighborList && !atoms.empty()) {
        box.grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());
    }

    // расчет нековалентных сил для каждого атома
    if (neighborList)
        ComputeForces<true>(atoms, box, neighborList);
    else
        ComputeForces<false>(atoms, box, neighborList);

    // проверка образования и разрыва связей, а также расчет сил
    std::erase_if(Bond::bonds_list, [&](Bond& bond) {
        if (bond.shouldBreak(atoms)) {
            Bond::BreakBond(&bond, atoms);
            return true;
        }
        
        bond.forceBond(atoms, dt);
        return false;
    });

    if (Bond::bonds_list.size() < 2) return;

    std::vector<std::uint16_t> degree(atoms.size(), 0);
    for (const Bond& bond : Bond::bonds_list) {
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

    for (const Bond& bond : Bond::bonds_list) {
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

template<bool UseNeighborList>
void ForceField::ComputeForces(AtomStorage& atoms, SimBox& box, NeighborList* neighborList) const {
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

        // взаимодействия с соседями
        if constexpr (UseNeighborList) {
            // используем список соседей
            for (uint32_t neighbourIndex : neighborList->neighborsIndices(atomIndex)) {
                pairNonBondedInteraction(atoms, neighbourIndex, ljPairRow, forceX, forceY, forceZ, posX, posY, posZ, potenE);
            }
        }
        else {
            // без списка соседей
            const int cx = box.grid.worldToCellX(posX);
            const int cy = box.grid.worldToCellY(posY);
            const int cz = box.grid.worldToCellZ(posZ);

            const int x0 = std::max(cx - 1, 0);
            const int x1 = std::min(cx + 1, box.grid.sizeX - 1);
            const int y0 = std::max(cy - 1, 0);
            const int y1 = std::min(cy + 1, box.grid.sizeY - 1);
            const int z0 = std::max(cz - 1, 0);
            const int z1 = std::min(cz + 1, box.grid.sizeZ - 1);

            for (int ix = x0; ix <= x1; ++ix) {
                for (int iy = y0; iy <= y1; ++iy) {
                    for (int iz = z0; iz <= z1; ++iz) {
                        for (uint32_t neighbourIndex : box.grid.atomsInCell(ix, iy, iz)) {
                            if (atomIndex <= neighbourIndex) continue;
                            pairNonBondedInteraction(atoms, neighbourIndex, ljPairRow, forceX, forceY, forceZ, posX, posY, posZ, potenE);
                        }
                    }
                }
            }
        }

        // записываем обратно в AtomStorage
        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
        atoms.energy(atomIndex) = potenE;
    }
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
