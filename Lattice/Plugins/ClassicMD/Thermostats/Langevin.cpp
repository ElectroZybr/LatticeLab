#include "Langevin.h"

#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"

void Langevin::pipeline(StepContext& stepContext) const {
    Verlet{}.pipeline(stepContext);
}
