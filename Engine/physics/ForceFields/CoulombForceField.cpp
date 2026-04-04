#include "CoulombForceField.h"

#include "Engine/NeighborSearch/NeighborList.h"
#include "Engine/metrics/Profiler.h"

void CoulombForceField::compute(AtomStorage& atoms, SimBox& box, NeighborList& neighborList) const {
    PROFILE_SCOPE("ForceField::Coulomb");
    (void)atoms;
    (void)box;
    (void)neighborList;
}
