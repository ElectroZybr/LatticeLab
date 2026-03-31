#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float accelDamping, float dt) const {
    VerletScheme{}.pipeline(atomStorage, box, forceField, neighborList, accelDamping, dt);
}
