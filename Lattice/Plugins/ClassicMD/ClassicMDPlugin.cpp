#include "ClassicMDPlugin.h"

#include "Lattice/Plugins/ClassicMD/ForceFields/ClassicMDForceField.h"
#include "Lattice/Plugins/ClassicMD/Integrators/KDK.h"
#include "Lattice/Plugins/ClassicMD/Integrators/RK4.h"
#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/Andersen.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/Langevin.h"

namespace {
template <typename T>
ModuleMeta<IForceField> makeForceFieldMeta() {
    return ModuleMeta<IForceField>{
        .id = std::string(T::id),
        .description = std::string(T::description),
        .factory = []() -> std::unique_ptr<IForceField> {
            return std::make_unique<T>();
        },
    };
}

template <typename T>
ModuleMeta<IIntegrator> makeIntegratorMeta() {
    return ModuleMeta<IIntegrator>{
        .id = std::string(T::id),
        .description = std::string(T::description),
        .factory = []() -> std::unique_ptr<IIntegrator> {
            return std::make_unique<T>();
        },
    };
}

template <typename T>
ModuleMeta<IThermostat> makeThermostatMeta() {
    return ModuleMeta<IThermostat>{
        .id = std::string(T::id),
        .description = std::string(T::description),
        .factory = []() -> std::unique_ptr<IThermostat> {
            return std::make_unique<T>();
        },
    };
}
} // namespace

void registerClassicMDPlugin() {
    static const bool registered = []() {
        registerClassicMDPlugin(ForceField::registry());
        registerClassicMDPlugin(Integrator::registry());
        registerClassicMDPlugin(Thermostat::registry());
        return true;
    }();
    (void)registered;
}

void registerClassicMDPlugin(ModuleRegistry<IForceField>& registry) {
    registry.add(makeForceFieldMeta<ClassicMDForceField>());
}

void registerClassicMDPlugin(ModuleRegistry<IIntegrator>& registry) {
    registry.add(makeIntegratorMeta<Verlet>());
    registry.add(makeIntegratorMeta<KDK>());
    registry.add(makeIntegratorMeta<RK4>());
}

void registerClassicMDPlugin(ModuleRegistry<IThermostat>& registry) {
    registry.add(makeThermostatMeta<Andersen>());
}
