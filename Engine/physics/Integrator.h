#pragma once

#include <cstdint>
#include <variant>

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

#include "Bond.h"
#include "integrators/KDKScheme.h"
#include "integrators/LangevinScheme.h"
#include "integrators/RK4Scheme.h"
#include "integrators/VerletScheme.h"

class Integrator {
public:
    enum class Scheme: uint8_t {
        Verlet,      // классический Velocity Verlet: устойчивый и быстрый базовый выбор
        KDK,         // Kick-Drift-Kick: симплектическая схема, удобна для поэтапного обновления сил
        RK4,         // Runge-Kutta 4-го порядка: высокая точность на шаг, но дороже по вычислениям
        Langevin,    // стохастический интегратор с термостатом (трение + случайный шум)
    };

    Integrator();

    void setScheme(Scheme scheme);
    Scheme getScheme() const { return integrator_type; }
    void setMaxParticleSpeed(float maxSpeed);
    float maxParticleSpeed() const { return maxParticleSpeed_; }
    void setAccelDamping(float accelDamping);
    float accelDamping() const { return accelDamping_; }

    void step(AtomStorage& atomStorage, Bond::List& bonds, SimBox& box, ForceField& forceField, NeighborList& neighborList, bool allowBondFormation, float dt);

private:
    using SchemeVariant = std::variant<VerletScheme, KDKScheme, RK4Scheme, LangevinScheme>;

    static SchemeVariant makeSchemeImpl(Scheme scheme);

    Scheme integrator_type = Scheme::Verlet;
    SchemeVariant scheme_impl;
    float maxParticleSpeed_ = 0.0f;
    float accelDamping_ = 0.9f;
};
