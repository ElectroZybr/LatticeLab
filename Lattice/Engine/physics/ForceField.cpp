#include "ForceField.h"

#include <algorithm>
#include <vector>

#include "Engine/World.h"
#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/metrics/Profiler.h"
#include "Engine/physics/Atom/AtomStorage.h"

namespace {
    using ExclusionList = std::vector<std::vector<uint32_t>>;

    ExclusionList buildExclusionList(const AtomStorage& atoms, const Bond::List& bonds) {
        ExclusionList excluded(atoms.size());

        for (const Bond& bond : bonds) {
            if (bond.aIndex >= atoms.size() || bond.bIndex >= atoms.size()) {
                continue;
            }
            excluded[bond.aIndex].push_back(static_cast<uint32_t>(bond.bIndex));
            excluded[bond.bIndex].push_back(static_cast<uint32_t>(bond.aIndex));
        }

        // Exclude 1-3 pairs as well: atoms connected through a common bonded center.
        for (size_t center = 0; center < excluded.size(); ++center) {
            auto& bonded = excluded[center];
            std::sort(bonded.begin(), bonded.end());
            bonded.erase(std::unique(bonded.begin(), bonded.end()), bonded.end());

            for (size_t i = 0; i + 1 < bonded.size(); ++i) {
                for (size_t j = i + 1; j < bonded.size(); ++j) {
                    excluded[bonded[i]].push_back(bonded[j]);
                    excluded[bonded[j]].push_back(bonded[i]);
                }
            }
        }

        for (auto& row : excluded) {
            std::sort(row.begin(), row.end());
            row.erase(std::unique(row.begin(), row.end()), row.end());
        }

        return excluded;
    }

    bool isExcludedPair(const ExclusionList& excluded, uint32_t aIndex, uint32_t bIndex) {
        if (aIndex >= excluded.size()) {
            return false;
        }
        const auto& row = excluded[aIndex];
        return std::binary_search(row.begin(), row.end(), bIndex);
    }

    template <bool UseLJ, bool UseCoulomb>
    void computePairInteractionsImpl(AtomStorage& atoms, const NeighborList& neighborList, const ExclusionList& excluded,
                                     const LJForceField& ljForceField, const CoulombForceField& coulombForceField) {
        const auto& offsets = neighborList.offsets();
        const auto& neighbours = neighborList.neighbors();

        for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
            const uint32_t begin = offsets[atomIndex];
            const uint32_t end = offsets[atomIndex + 1];
            if (begin > end || static_cast<size_t>(end) > neighbours.size()) {
                continue;
            }

            const float posX = atoms.posX(atomIndex);
            const float posY = atoms.posY(atomIndex);
            const float posZ = atoms.posZ(atomIndex);
            float forceX = atoms.forceX(atomIndex);
            float forceY = atoms.forceY(atomIndex);
            float forceZ = atoms.forceZ(atomIndex);
            float potentialEnergy = atoms.energy(atomIndex);

            const LJForceField::LJPairRow* ljPairRow = nullptr;
            if constexpr (UseLJ) {
                ljPairRow = &ljForceField.pairRow(atoms.type(atomIndex));
            }

            float charge = 0.0f;
            if constexpr (UseCoulomb) {
                charge = atoms.charge(atomIndex);
                if (charge == 0.0f) {
                    if constexpr (!UseLJ) {
                        continue;
                    }
                }
            }

            for (uint32_t p = begin; p < end; ++p) {
                const uint32_t bIndex = neighbours[p];
                if (isExcludedPair(excluded, static_cast<uint32_t>(atomIndex), bIndex)) {
                    continue;
                }

                const float dx = atoms.posX(bIndex) - posX;
                const float dy = atoms.posY(bIndex) - posY;
                const float dz = atoms.posZ(bIndex) - posZ;
                const float d2 = dx * dx + dy * dy + dz * dz;

                if constexpr (UseLJ) {
                    ljForceField.pairInteraction(atoms, bIndex, dx, dy, dz, d2, *ljPairRow, forceX, forceY, forceZ, potentialEnergy);
                }
                if constexpr (UseCoulomb) {
                    if (charge != 0.0f) {
                        coulombForceField.pairInteraction(atoms, bIndex, dx, dy, dz, d2, charge, forceX, forceY, forceZ, potentialEnergy);
                    }
                }
            }

            atoms.forceX(atomIndex) = forceX;
            atoms.forceY(atomIndex) = forceY;
            atoms.forceZ(atomIndex) = forceZ;
            atoms.energy(atomIndex) = potentialEnergy;
        }
    }
}

bool ForceField::compute(World& world, bool allowBondFormation, float dt) const {
    PROFILE_SCOPE("ForceField::compute");

    AtomStorage& atoms = world.getAtomStorage();
    Bond::List& bonds = world.getBonds();

    // расчет сил стен и гравитации
    wallForceField_.compute(world);
    computePairInteractions(world);

    // расчет дальнодействующих кулоновских сил
    if (world.isCoulombEnabled() && world.isCoulombLongRangeEnabled()) {
        coulombForceField_.computeLongRange(atoms, world.getGrid());
    }

    return bondForceField_.compute(atoms, bonds, world.getNeighborList(), allowBondFormation, dt);
}

void ForceField::computePairInteractions(World& world) const {
    AtomStorage& atoms = world.getAtomStorage();
    NeighborList& neighborList = world.getNeighborList();
    const ExclusionList excluded = buildExclusionList(atoms, world.getBonds());

    if (world.isLJEnabled() && world.isCoulombEnabled()) {
        computePairInteractionsImpl<true, true>(atoms, neighborList, excluded, ljForceField_, coulombForceField_);
    }
    else if (world.isLJEnabled()) {
        computePairInteractionsImpl<true, false>(atoms, neighborList, excluded, ljForceField_, coulombForceField_);
    }
    else {
        computePairInteractionsImpl<false, true>(atoms, neighborList, excluded, ljForceField_, coulombForceField_);
    }
}
