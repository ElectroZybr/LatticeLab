#pragma once

#include "Lattice/Engine/physics/Integrator.h"

class Langevin final : public IIntegrator {
public:
    static constexpr std::string_view id = "langevin";
    static constexpr std::string_view description = "integrator_langevin";

    void step(StepContext& stepContext) override { pipeline(stepContext); }

    void pipeline(StepContext& stepContext) const;
};
