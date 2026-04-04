#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(AtomStorage& atomStorage, Bond::List& bonds, SimBox& box, ForceField& forceField, NeighborList* neighborList, bool allowBondFormation, float accelDamping, float dt) const {
    VerletScheme{}.pipeline(atomStorage, bonds, box, forceField, neighborList, allowBondFormation, accelDamping, dt);
}
