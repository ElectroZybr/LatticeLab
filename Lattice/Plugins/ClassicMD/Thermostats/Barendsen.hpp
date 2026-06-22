#pragma once 

#include <algorithm>
#include <random>

#include "Lattice/Engine/physics/IThermostat.h"

class Barendsen final : public IThermostat {
public:
    static constexpr std::string_view id = "barendsen";
    static constexpr std::string_view description = "thermostat_barendsen";

    Barendsen(double temperature = 300.0, double param = 5.0)  :
        t(temperature), nu(param) {}

    float temperature() const override { return static_cast<float>(t); }
    void setTemperature(float temperature) override { t = temperature; }
    void setParam(float param) override { nu = std::max(0.0f, param); }
    float param() const override { return static_cast<float>(nu); }
    void apply(StepContext& stepContext) override;

private:
    double t;
    double nu;
};
