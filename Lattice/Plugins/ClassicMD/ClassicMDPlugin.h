#pragma once

#include "Lattice/Engine/physics/IForceField.h"
#include "Lattice/Engine/physics/IIntegrator.h"
#include "Lattice/Engine/physics/IThermostat.h"

void registerClassicMDPlugin();
void registerClassicMDPlugin(ModuleRegistry<IForceField>& registry);
void registerClassicMDPlugin(ModuleRegistry<IIntegrator>& registry);
void registerClassicMDPlugin(ModuleRegistry<IThermostat>& registry);
