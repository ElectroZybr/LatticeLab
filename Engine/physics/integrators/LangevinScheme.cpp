#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(StepContext& ctx) const {
    // TODO: dedicated Langevin implementation (friction + stochastic term).
    VerletScheme{}.pipeline(ctx);
}
