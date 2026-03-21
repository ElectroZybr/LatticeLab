#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // TODO: dedicated Langevin implementation (friction + stochastic term).
    VerletScheme{}.pipeline(atoms, box, forceField, dt);
}
