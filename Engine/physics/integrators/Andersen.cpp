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
    AtomStorage& atomStorage = stepData.world.getAtomStorage();
    double probability = stepData.dt * nu;
    std::uniform_real_distribution<double> uniDist(0.0, 1.0);
    size_t N = atomStorage.size(); 

    for(int i = 0; i < N; ++i)
    {
        double randomVal = uniDist(randomGenerator);

        if(randomVal < probability)
        {
            float m = AtomData::getProps(atomStorage.type(i)).mass;
            float sigma = std::sqrt(Units::kboltzmann * t / m);
            std::normal_distribution<float> Maksvell(0.f, sigma);

            atomStorage.velX(i) = Maksvell(randomGenerator);
            atomStorage.velY(i) = Maksvell(randomGenerator);
            atomStorage.velZ(i) = Maksvell(randomGenerator);
        }
    }
}
