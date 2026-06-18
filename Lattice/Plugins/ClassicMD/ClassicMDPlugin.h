#pragma once

#include "Lattice/Engine/physics/Integrator.h"
#include "Lattice/Engine/physics/Thermostat.h"

void registerClassicMDPlugin();
void registerClassicMDPlugin(IntegratorRegistry& registry);
void registerClassicMDPlugin(ThermostatRegistry& registry);
