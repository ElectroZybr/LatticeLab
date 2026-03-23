#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box,
                              ForceField& forceField, double dt) const {
    VerletScheme{}.pipeline(atomStorage, atoms, box, forceField, dt);
}
