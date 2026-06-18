#pragma once

#include "Lattice/Engine/physics/Integrator.h"

class RK4 final : public IIntegrator {
public:
    static constexpr std::string_view id = "rk4";
    static constexpr std::string_view description = "integrator_runge_kutta_4";

    void step(StepData& stepData) override { pipeline(stepData); }

    void pipeline(StepData& stepData) const;
};
