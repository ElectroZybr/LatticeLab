#pragma once

#include <string_view>

#include "CreateDebugPanels.h"
#include "UpdateDebugData.h"
#include "Engine/Simulation.h"
#include "Engine/physics/Integrator.h"

inline std::string_view integratorSchemeName(Integrator::Scheme scheme) {
    switch (scheme) {
        case Integrator::Scheme::Verlet:   return "Velocity Verlet";
        case Integrator::Scheme::KDK:      return "KDK (Kick-Drift-Kick)";
        case Integrator::Scheme::RK4:      return "Runge-Kutta 4";
        case Integrator::Scheme::Langevin: return "Langevin";
    }
    return "Unknown";
}

inline void refreshAtomDebugViews(const DebugViews& debugViews, const Simulation& simulation) {
    updateAtomSelectionDebug(debugViews, simulation);
}

inline void refreshSimulationDebugViews(const DebugViews& debugViews, const Simulation& simulation) {
    updateSimulationDebug(debugViews, simulation, integratorSchemeName(simulation.getIntegrator()));
}
