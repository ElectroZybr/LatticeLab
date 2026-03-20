#include "ForceField.h"

#include "Atom.h"
#include "../SimBox.h"

void ForceField::compute(std::vector<Atom>& atoms, SimBox& box, double dt) const {
    (void)atoms;
    (void)box;
    (void)dt;
}
