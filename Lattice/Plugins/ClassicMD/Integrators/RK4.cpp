#include "RK4.h"

#include "Lattice/Engine/physics/integrators/RK4Scheme.h"

REGISTER_INTEGRATOR(RK4)

void RK4::pipeline(StepData& stepData) const {
    RK4Scheme{}.pipeline(stepData);
}
