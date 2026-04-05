#include "LJForceField.h"

#include <cmath>

#include "Engine/Consts.h"
#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/metrics/Profiler.h"

LJForceField::LJForceField() : ljPairTable_(buildLJPairTable()) {}

LJForceField::LJPairTable LJForceField::buildLJPairTable() {
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

            params.potentialC6 = 4.0f * eps * a6;
            params.potentialC12 = 4.0f * eps * a12;

            table[i][j] = params;
            table[j][i] = params;
        }
    }
    return table;
}

void LJForceField::compute(AtomStorage& atoms, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::ComputeForces(NL)");
    const auto& offsets = neighborList.offsets();
    const auto& neighbours = neighborList.neighbors();

    for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
        // используем список соседей
        if (atomIndex + 1 >= offsets.size()) continue;

        const uint32_t begin = offsets[atomIndex];
        const uint32_t end = offsets[atomIndex + 1];
        if (begin > end || static_cast<size_t>(end) > neighbours.size()) continue;

        // загружаем данные текущего атома из AtomStorage
        float posX = atoms.posX(atomIndex);
        float posY = atoms.posY(atomIndex);
        float posZ = atoms.posZ(atomIndex);
        float forceX = atoms.forceX(atomIndex);
        float forceY = atoms.forceY(atomIndex);
        float forceZ = atoms.forceZ(atomIndex);
        float potenE = atoms.energy(atomIndex);
        // выбираем строку таблицы LJ для данного типа атома
        const LJPairRow& ljPairRow = ljPairTable_[static_cast<size_t>(atoms.type(atomIndex))];

        for (uint32_t p = begin; p < end; ++p) {
            pairInteraction(atoms, neighbours[p], ljPairRow, forceX, forceY, forceZ, posX, posY, posZ, potenE);
        }

        // записываем обратно в AtomStorage
        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
        atoms.energy(atomIndex) = potenE;
    }
}

void LJForceField::pairInteraction(AtomStorage& atoms, uint32_t bIndex, const LJPairRow& ljPairRow,
                                   float& forceX, float& forceY, float& forceZ,
                                   float posX, float posY, float posZ, float& potenE) const {
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

    const float term6  = params.potentialC6 * invD6;
    const float term12 = params.potentialC12 * invD12;

    const float forceScale = (12.0f * term12 - 6.0f * term6) * invD2;
    const float potential  = term12 - term6;

    // расчет сил LJ
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

    potenE += 0.5f * potential;
    atoms.energy(bIndex) += 0.5f * potential;
}
