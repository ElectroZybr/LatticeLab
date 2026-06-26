#include "PluginInit.h"

#include "Lattice/Plugins/ClassicMD/ForceFields/ClassicMDForceField.h"
#include "Lattice/Plugins/ClassicMD/Integrators/KDK.h"
#include "Lattice/Plugins/ClassicMD/Integrators/RK4.h"
#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/AccelDamp.hpp"
#include "Lattice/Plugins/ClassicMD/Thermostats/Andersen.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/Barendsen.hpp"

extern "C" bool plugin_init(PluginHost& host, PluginInfo& info) {
    info.id = "classic_md";
    info.name = "Classic Molecular Dynamics";
    info.version = "0.1.0";

    host.forceFields.add(makeModuleMeta<IForceField, ClassicMDForceField>());
    host.integrators.add(makeModuleMeta<IIntegrator, Verlet>());
    host.integrators.add(makeModuleMeta<IIntegrator, KDK>());
    host.integrators.add(makeModuleMeta<IIntegrator, RK4>());
    host.thermostats.add(makeModuleMeta<IThermostat, AccelDamp>());
    host.thermostats.add(makeModuleMeta<IThermostat, Andersen>());
    host.thermostats.add(makeModuleMeta<IThermostat, Barendsen>());
    return true;
}
