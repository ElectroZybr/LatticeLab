#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, double dt) const {
    VerletScheme{}.pipeline(atomStorage, box, forceField, dt);
}
