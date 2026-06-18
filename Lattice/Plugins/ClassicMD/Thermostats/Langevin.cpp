#include "Langevin.h"

#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"

REGISTER_INTEGRATOR(Langevin)

void Langevin::pipeline(StepContext& stepContext) const {
    Verlet{}.pipeline(stepContext);
}
