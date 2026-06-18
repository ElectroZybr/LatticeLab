#include "Lattice/Engine/physics/Thermostat.h"

#include <algorithm>

ThermostatRegistry& globalThermostatRegistry() {
    static ThermostatRegistry registry;
    return registry;
}

bool Thermostat::setThermostat(std::string_view id) {
    if (id.empty()) {
        currentId_.clear();
        impl_.reset();
        return true;
    }

    const ThermostatMeta* meta = globalThermostatRegistry().find(id);
    if (meta == nullptr || meta->factory == nullptr) {
        return false;
    }

    std::unique_ptr<IThermostat> impl = meta->factory();
    if (!impl) {
        return false;
    }

    impl->setTemperature(temperature_);
    currentId_ = meta->id;
    impl_ = std::move(impl);
    return true;
}

void Thermostat::setTemperature(float temperature) {
    temperature_ = std::max(0.0f, temperature);
    if (impl_) {
        impl_->setTemperature(temperature_);
    }
}
