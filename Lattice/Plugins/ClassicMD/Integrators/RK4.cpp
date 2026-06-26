#include "RK4.h"

#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"

void RK4::pipeline(StepContext& stepContext) const {
    Verlet{}.pipeline(stepContext);
}
