#include "LangevinScheme.h"

#include "Lattice/Engine/physics/integrators/VerletScheme.h"

void LangevinScheme::pipeline(StepData& stepData) const { VerletScheme{}.pipeline(stepData); }
