#pragma once 

#include <random>

#include "Lattice/Engine/physics/Thermostat.h"

class Andersen final : public IThermostat {
public:
    static constexpr std::string_view id = "andersen";
    static constexpr std::string_view description = "thermostat_andersen";

    Andersen(double temperature = 300.0,
             double param = 0.1,
             std::mt19937::result_type seed = std::mt19937::default_seed)  :
        t(temperature),
        nu(param),
        randomGenerator(seed)
    {}

    float temperature() const override { return static_cast<float>(t); }
    double getParam() const { return nu; }
    void setTemperature(float temperature) override { t = temperature; }
    void apply(StepContext& stepContext) override;

    void mkMove(StepContext& stepContext);

private:

    double t;
    double nu;
    std::mt19937 randomGenerator;
};
