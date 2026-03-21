#include "RK4Scheme.h"

#include "VerletScheme.h"

void RK4Scheme::pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // TODO: dedicated RK4 implementation (k1/k2/k3/k4 with temporary states).
    VerletScheme{}.pipeline(atoms, box, forceField, dt);
}
