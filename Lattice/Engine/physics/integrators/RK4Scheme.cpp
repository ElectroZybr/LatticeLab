#include "RK4Scheme.h"

#include "Lattice/Engine/physics/integrators/VerletScheme.h"

void RK4Scheme::pipeline(StepData& stepData) const { VerletScheme{}.pipeline(stepData); }
