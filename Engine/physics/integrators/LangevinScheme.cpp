#include "LangevinScheme.h"

#include "VerletScheme.h"

void LangevinScheme::pipeline(StepData& stepData) const {
    VerletScheme{}.pipeline(stepData);
}
