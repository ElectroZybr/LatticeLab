#include "CoulombForceField.h"

#include <cmath>

#include "Engine/Consts.h"
#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/metrics/Profiler.h"

namespace {
    constexpr float kCoulombEvAngstrom = 14.399645f; // eV*A/e^2

    void pairInteraction(AtomStorage& atoms, uint32_t bIndex, float chargeA, float& forceX, float& forceY, float& forceZ, float posX,
                         float posY, float posZ, float& potentialEnergy) {
        if (std::abs(chargeA) <= Consts::Epsilon) {
            return;
        }

        const float chargeB = atoms.charge(bIndex);
        if (std::abs(chargeB) <= Consts::Epsilon) {
            return;
        }

        const float dx = atoms.posX(bIndex) - posX;
        const float dy = atoms.posY(bIndex) - posY;
        const float dz = atoms.posZ(bIndex) - posZ;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= Consts::Epsilon) {
            return;
        }

        const float invR = 1.0f / std::sqrt(d2);
        const float invR3 = invR * invR * invR;
        const float qq = chargeA * chargeB;
        const float forceScale = kCoulombEvAngstrom * qq * invR3;
        const float potential = kCoulombEvAngstrom * qq * invR;

        const float pairForceX = dx * forceScale;
        const float pairForceY = dy * forceScale;
        const float pairForceZ = dz * forceScale;

        forceX -= pairForceX;
        forceY -= pairForceY;
        forceZ -= pairForceZ;

        atoms.forceX(bIndex) += pairForceX;
        atoms.forceY(bIndex) += pairForceY;
        atoms.forceZ(bIndex) += pairForceZ;

        potentialEnergy += 0.5f * potential;
        atoms.energy(bIndex) += 0.5f * potential;
    }
}

void CoulombForceField::compute(AtomStorage& atoms, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::Coulomb");
    const auto& offsets = neighborList.offsets();
    const auto& neighbours = neighborList.neighbors();

    for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
        if (atomIndex + 1 >= offsets.size()) {
            break;
        }

        const uint32_t begin = offsets[atomIndex];
        const uint32_t end = offsets[atomIndex + 1];
        if (begin > end || static_cast<size_t>(end) > neighbours.size()) {
            continue;
        }

        float posX = atoms.posX(atomIndex);
        float posY = atoms.posY(atomIndex);
        float posZ = atoms.posZ(atomIndex);
        float forceX = atoms.forceX(atomIndex);
        float forceY = atoms.forceY(atomIndex);
        float forceZ = atoms.forceZ(atomIndex);
        float potentialEnergy = atoms.energy(atomIndex);
        const float chargeA = atoms.charge(atomIndex);

        for (uint32_t p = begin; p < end; ++p) {
            pairInteraction(atoms, neighbours[p], chargeA, forceX, forceY, forceZ, posX, posY, posZ, potentialEnergy);
        }

        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
        atoms.energy(atomIndex) = potentialEnergy;
    }
}
