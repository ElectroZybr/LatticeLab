#pragma once

#include <algorithm>

#include "Lattice/Engine/physics/IThermostat.h"

class AccelDamp final : public IThermostat {
public:
    static constexpr std::string_view id = "accel_damp";
    static constexpr std::string_view description = "thermostat_accel_damp";

    AccelDamp(double temperature = 300.0, double param = 1.0) : t(temperature), tau(param) {}

    float temperature() const override { return static_cast<float>(t); }
    void setTemperature(float temperature) override { t = temperature; }
    void setParam(float param) override { tau = std::max(0.0f, param); }
    float param() const override { return static_cast<float>(tau); }
    void apply(StepContext& stepContext) override;

private:
    double t;
    double tau;
};
