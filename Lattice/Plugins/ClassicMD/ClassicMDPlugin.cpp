#include "ClassicMDPlugin.h"

#include "Lattice/Plugins/ClassicMD/Integrators/KDK.h"
#include "Lattice/Plugins/ClassicMD/Integrators/RK4.h"
#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/Andersen.h"
#include "Lattice/Plugins/ClassicMD/Thermostats/Langevin.h"

namespace {
template <typename T>
IntegratorMeta makeIntegratorMeta() {
    return IntegratorMeta{
        .id = std::string(T::id),
        .description = std::string(T::description),
        .factory = []() -> std::unique_ptr<IIntegrator> {
            return std::make_unique<T>();
        },
    };
}

template <typename T>
ThermostatMeta makeThermostatMeta() {
    return ThermostatMeta{
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
        registerClassicMDPlugin(globalIntegratorRegistry());
        registerClassicMDPlugin(globalThermostatRegistry());
        return true;
    }();
    (void)registered;
}

void registerClassicMDPlugin(IntegratorRegistry& registry) {
    registry.add(makeIntegratorMeta<Verlet>());
    registry.add(makeIntegratorMeta<KDK>());
    registry.add(makeIntegratorMeta<RK4>());
    registry.add(makeIntegratorMeta<Langevin>());
}

void registerClassicMDPlugin(ThermostatRegistry& registry) {
    registry.add(makeThermostatMeta<Andersen>());
}
