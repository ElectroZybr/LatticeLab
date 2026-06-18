#pragma once

#include "Lattice/Engine/physics/Integrator.h"

class KDK final : public IIntegrator {
public:
    static constexpr std::string_view id = "kdk";
    static constexpr std::string_view description = "integrator_kdk";

    void step(StepContext& stepContext) override { pipeline(stepContext); }

    void pipeline(StepContext& stepContext) const;
    static void halfKick(AtomStorage& atomStorage, float accelDamping, float dt);
    static void drift(AtomStorage& atomStorage, float dt);
};
