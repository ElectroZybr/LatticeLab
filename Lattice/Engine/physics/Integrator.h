#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class AtomStorage;
class ForceField;
class NeighborList;
class World;

struct StepData {
    World& world;
    ForceField& forceField;
    NeighborList& neighborList;
    bool allowBondFormation;
    bool bondsChanged = false;
    float accelDamping;
    float dt;
};

class IIntegrator;

class Integrator {
public:
    Integrator();

    bool setIntegrator(std::string_view id);
    std::string_view getIntegrator() const { return currentId_; }
    void setMaxParticleSpeed(float maxSpeed);
    float maxParticleSpeed() const { return maxParticleSpeed_; }
    void setAccelDamping(float accelDamping);
    float accelDamping() const { return accelDamping_; }

    void setAndersenTemperature(float temperature);
    float andersenTemperature() const;
    void step(StepData& stepData);

private:
    std::string currentId_ = "verlet";
    std::unique_ptr<IIntegrator> impl_;

    float maxParticleSpeed_ = 0.0f;
    float accelDamping_ = 0.9f;
    float andersenTemperature_ = 300.0f;
};

/// абстрактный интегратор
class IIntegrator {
public:
    virtual ~IIntegrator() = default;
    virtual void step(StepData& stepData) = 0;
};

using IntegratorFactory = std::unique_ptr<IIntegrator> (*)();

/// мета данные интеграторов
struct IntegratorMeta {
    std::string id;
    std::string description;
    IntegratorFactory factory = nullptr;
};

/// регистратор итеграторов
class IntegratorRegistry {
public:
    bool add(IntegratorMeta meta) {
        if (meta.id.empty() || meta.factory == nullptr || find(meta.id) != nullptr) {
            return false;
        }

        items_.push_back(std::move(meta));
        return true;
    }

    const IntegratorMeta* find(std::string_view id) const {
        for (const IntegratorMeta& item : items_) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

    std::unique_ptr<IIntegrator> create(std::string_view id) const {
        const IntegratorMeta* meta = find(id);
        return meta != nullptr && meta->factory != nullptr ? meta->factory() : nullptr;
    }

    const std::vector<IntegratorMeta>& items() const { return items_; }

private:
    std::vector<IntegratorMeta> items_{};
};

inline IntegratorRegistry& globalIntegratorRegistry() {
    static IntegratorRegistry registry;
    return registry;
}

#define LL_CONCAT_IMPL(a, b) a##b
#define LL_CONCAT(a, b) LL_CONCAT_IMPL(a, b)

#define REGISTER_INTEGRATOR(Type)                                                       \
    namespace {                                                                         \
        const bool LL_CONCAT(autoRegIntegrator_, __LINE__) = []() {                     \
            return globalIntegratorRegistry().add(IntegratorMeta{                       \
                std::string(Type::id),                                                  \
                std::string(Type::description),                                         \
                []() -> std::unique_ptr<IIntegrator> { return std::make_unique<Type>(); } \
            });                                                                         \
        }();                                                                            \
    }
