#include "AndersenIntegrator.h"

#include "Lattice/Plugins/ClassicMD/Thermostats/Andersen.h"

REGISTER_INTEGRATOR(AndersenIntegrator)

void AndersenIntegrator::pipeline(StepData& stepData) {
    Andersen{temperature_, nu_}.pipeline(stepData);
}
