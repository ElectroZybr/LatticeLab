#include "Langevin.h"

#include "Lattice/Engine/physics/integrators/LangevinScheme.h"

REGISTER_INTEGRATOR(Langevin)

void Langevin::pipeline(StepData& stepData) const {
    LangevinScheme{}.pipeline(stepData);
}
