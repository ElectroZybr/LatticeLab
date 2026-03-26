#pragma once

#include <string_view>

class Simulation;

class SimulationStateIO {
public:
    static void save(const Simulation& simulation, std::string_view path);
    static void load(Simulation& simulation, std::string_view path);
};
