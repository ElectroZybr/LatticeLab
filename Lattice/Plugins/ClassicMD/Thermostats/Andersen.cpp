#include "Andersen.h"

#include <algorithm>
#include <cmath>

#include "Lattice/Engine/Consts.h"
#include "Lattice/Engine/World.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"
#include "Lattice/Engine/physics/Integrator.h"

REGISTER_THERMOSTAT(Andersen)

void Andersen::apply(StepContext& stepContext)
{
    PROFILE_SCOPE("Andersen::apply");
    mkMove(stepContext);
}

void Andersen::mkMove(StepContext& stepContext)
{
    AtomStorage& atomStorage = stepContext.world.getAtomStorage();
    const double probability = std::clamp(stepContext.dt * nu, 0.0, 1.0);
    std::uniform_real_distribution<double> uniDist(0.0, 1.0);
    const size_t mobileCount = atomStorage.mobileCount();

    for (size_t i = 0; i < mobileCount; ++i)
    {
        const double randomVal = uniDist(randomGenerator);

        if(randomVal < probability)
        {
            const float m = AtomData::getProps(atomStorage.type(i)).mass;
            const float sigma = std::sqrt(Units::kboltzmann * t / m);
            std::normal_distribution<float> Maksvell(0.f, sigma);

            atomStorage.velX(i) = Maksvell(randomGenerator);
            atomStorage.velY(i) = Maksvell(randomGenerator);
            atomStorage.velZ(i) = Maksvell(randomGenerator);
        }
    }
}
