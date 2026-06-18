#include "ClassicMDPlugin.h"

#include "Lattice/Plugins/ClassicMD/Integrators/KDK.h"
#include "Lattice/Plugins/ClassicMD/Integrators/RK4.h"
#include "Lattice/Plugins/ClassicMD/Integrators/Verlet.h"
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
} // namespace

void registerClassicMDPlugin(IntegratorRegistry& registry) {
    registry.add(makeIntegratorMeta<Verlet>());
    registry.add(makeIntegratorMeta<KDK>());
    registry.add(makeIntegratorMeta<RK4>());
    registry.add(makeIntegratorMeta<Langevin>());
}
