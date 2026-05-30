#include "LangevinScheme.h"

#include "Lattice/physics/integrators/VerletScheme.h"

void LangevinScheme::pipeline(StepData& stepData) const { VerletScheme{}.pipeline(stepData); }
