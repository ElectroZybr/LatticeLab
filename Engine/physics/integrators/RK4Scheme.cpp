#include "RK4Scheme.h"

#include "VerletScheme.h"

void RK4Scheme::pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField,
                         double dt) const {
    VerletScheme{}.pipeline(atomStorage, atoms, box, forceField, dt);
}
