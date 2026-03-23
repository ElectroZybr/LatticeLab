#include "RK4Scheme.h"

#include "VerletScheme.h"

void RK4Scheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const {
    VerletScheme{}.pipeline(atomStorage, box, forceField, dt);
}
