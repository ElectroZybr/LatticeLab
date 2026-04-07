#include "ForceField.h"

#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/metrics/Profiler.h"

ForceField::ForceField() = default;

void ForceField::syncWalls(const SimBox& box) { wallForceField_.syncWalls(box); }

void ForceField::compute(AtomStorage& atoms, Bond::List& bonds, SimBox& box, NeighborList& neighborList, bool allowBondFormation, float dt) const {
    PROFILE_SCOPE("ForceField::compute");

    wallForceField_.compute(atoms, static_force_);
    computePairInteractions(atoms, neighborList);
    // bondForceField_.compute(atoms, bonds, neighborList, allowBondFormation, dt);
}

void ForceField::computePairInteractions(AtomStorage& atoms, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::pairInteractions");
    const auto& offsets = neighborList.offsets();
    const auto& neighbours = neighborList.neighbors();

    for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
        const uint32_t begin = offsets[atomIndex];
        const uint32_t end = offsets[atomIndex + 1];
        if (begin > end || static_cast<size_t>(end) > neighbours.size()) {
            continue;
        }

        // загружаем данные атома
        const float posX = atoms.posX(atomIndex);
        const float posY = atoms.posY(atomIndex);
        const float posZ = atoms.posZ(atomIndex);
        float forceX = atoms.forceX(atomIndex);
        float forceY = atoms.forceY(atomIndex);
        float forceZ = atoms.forceZ(atomIndex);
        float potentialEnergy = atoms.energy(atomIndex);
        const LJForceField::LJPairRow* ljPairRow = enableLJ_ ? &ljForceField_.pairRow(atoms.type(atomIndex)) : nullptr;
        const float charge = atoms.charge(atomIndex);
        const bool computeCoulomb = enableCoulomb_ && charge != 0.0f;

        for (uint32_t p = begin; p < end; ++p) {
            const uint32_t bIndex = neighbours[p];
            const float dx = atoms.posX(bIndex) - posX;
            const float dy = atoms.posY(bIndex) - posY;
            const float dz = atoms.posZ(bIndex) - posZ;
            const float d2 = dx * dx + dy * dy + dz * dz;

            if (enableLJ_) {
                // парные LJ-взаимодействия
                ljForceField_.pairInteraction(atoms, bIndex, dx, dy, dz, d2, *ljPairRow, forceX, forceY, forceZ, potentialEnergy);
            }
            if (computeCoulomb) {
                // парные кулоновские взаимодействия (близкодействующие силы)
                coulombForceField_.pairInteraction(atoms, bIndex, dx, dy, dz, d2, charge, forceX, forceY, forceZ, potentialEnergy);
            }
        }

        // сохраняем накопленные силы и энергию
        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
        atoms.energy(atomIndex) = potentialEnergy;
    }
}
