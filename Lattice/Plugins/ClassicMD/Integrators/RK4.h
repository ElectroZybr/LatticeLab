#pragma once

#include "Lattice/Engine/physics/Integrator.h"

class RK4 final : public IIntegrator {
public:
    static constexpr std::string_view id = "rk4";
    static constexpr std::string_view description = "integrator_runge_kutta_4";

    void step(StepContext& stepContext) override { pipeline(stepContext); }

    void pipeline(StepContext& stepContext) const;
};
