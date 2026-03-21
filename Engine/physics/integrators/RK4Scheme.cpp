#include "RK4Scheme.h"

#include "VerletScheme.h"

void RK4Scheme::pipeline(StepContext& ctx) const {
    // TODO: dedicated RK4 implementation (k1/k2/k3/k4 with temporary states).
    VerletScheme{}.pipeline(ctx);
}
