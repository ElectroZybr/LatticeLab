#include "Andersen.h"

#include "Engine/metrics/Profiler.h"
#include "Engine/physics/integrators/StepOps.h"
#include "Engine/physics/integrators/VerletScheme.h"

void Andersen::pipeline(StepData& stepData)
{
    PROFILE_SCOPE("Andersen::pipeline");
    
    StepOps::predictAndSync(stepData, &VerletScheme::predict);
    StepOps::computeForces(stepData);
    VerletScheme::correct(stepData.world.getAtomStorage(), stepData.accelDamping, stepData.dt);
    mkMove(stepData);
}

void Andersen::mkMove(StepData& stepData)
{
    
}
