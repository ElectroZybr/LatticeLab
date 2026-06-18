#pragma once

#include "Lattice/Engine/physics/Integrator.h"

class AndersenIntegrator final : public IIntegrator {
public:
    static constexpr std::string_view id = "andersen";
    static constexpr std::string_view description = "integrator_andersen";

    void step(StepData& stepData) override { pipeline(stepData); }

    void pipeline(StepData& stepData);

private:
    double temperature_ = 300.0;
    double nu_ = 0.1;
};
