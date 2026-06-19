#pragma once

#include <string_view>

#include "App/debug/CreateDebugPanels.h"
#include "App/debug/UpdateDebugData.h"
#include "App/localization/i18n.h"
#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/physics/IIntegrator.h"

inline std::string_view integratorSchemeName(std::string_view id) {
    const ModuleMeta<IIntegrator>* meta = Integrator::registry().find(id);
    if (meta != nullptr && !meta->description.empty()) {
        return i18n::tr(meta->description);
    }
    return id;
}

inline void refreshAtomDebugViews(const DebugViews& debugViews, const Lattice::Simulation& simulation) {
    updateAtomSelectionDebug(debugViews, simulation);
}

inline void refreshSimulationDebugViews(const DebugViews& debugViews, const Lattice::Simulation& simulation) {
    updateSimulationDebug(debugViews, simulation, integratorSchemeName(simulation.getIntegrator()));
}
