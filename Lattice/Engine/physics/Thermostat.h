#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Lattice/Engine/physics/StepContext.h"

class IThermostat {
public:
    virtual ~IThermostat() = default;
    virtual void setTemperature(float temperature) { (void)temperature; }
    virtual float temperature() const { return 0.0f; }
    virtual void apply(StepContext& stepContext) = 0;
};

using ThermostatFactory = std::unique_ptr<IThermostat> (*)();

struct ThermostatMeta {
    std::string id;
    std::string description;
    ThermostatFactory factory = nullptr;
};

class ThermostatRegistry {
public:
    bool add(ThermostatMeta meta) {
        if (meta.id.empty() || meta.factory == nullptr || find(meta.id) != nullptr) {
            return false;
        }

        items_.push_back(std::move(meta));
        return true;
    }

    const ThermostatMeta* find(std::string_view id) const {
        for (const ThermostatMeta& item : items_) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

    std::unique_ptr<IThermostat> create(std::string_view id) const {
        const ThermostatMeta* meta = find(id);
        return meta != nullptr && meta->factory != nullptr ? meta->factory() : nullptr;
    }

    const std::vector<ThermostatMeta>& items() const { return items_; }

private:
    std::vector<ThermostatMeta> items_{};
};

class Thermostat {
public:
    Thermostat() = default;
    ~Thermostat() = default;
    Thermostat(const Thermostat&) = delete;
    Thermostat& operator=(const Thermostat&) = delete;
    Thermostat(Thermostat&&) noexcept = default;
    Thermostat& operator=(Thermostat&&) noexcept = default;

    bool setThermostat(std::string_view id);
    std::string_view getThermostat() const { return currentId_; }
    IThermostat* activeThermostat() const { return impl_.get(); }

    void setTemperature(float temperature);
    float temperature() const { return temperature_; }

private:
    std::string currentId_{};
    std::unique_ptr<IThermostat> impl_;
    float temperature_ = 300.0f;
};

ThermostatRegistry& globalThermostatRegistry();

#ifndef LL_CONCAT_IMPL
#define LL_CONCAT_IMPL(a, b) a##b
#endif

#ifndef LL_CONCAT
#define LL_CONCAT(a, b) LL_CONCAT_IMPL(a, b)
#endif

#define REGISTER_THERMOSTAT(Type)                                                      \
    namespace {                                                                        \
        const bool LL_CONCAT(autoRegThermostat_, __LINE__) = []() {                    \
            return globalThermostatRegistry().add(ThermostatMeta{                      \
                std::string(Type::id),                                                 \
                std::string(Type::description),                                        \
                []() -> std::unique_ptr<IThermostat> { return std::make_unique<Type>(); } \
            });                                                                        \
        }();                                                                           \
    }
