#pragma once

#include <variant>
#include <vector>

class Atom;
class ForceField;
class SimBox;

#include "integrators/KDKScheme.h"
#include "integrators/LangevinScheme.h"
#include "integrators/RK4Scheme.h"
#include "integrators/VerletScheme.h"

class Integrator {
public:
    enum class Scheme {
        Verlet,      // классический Velocity Verlet: устойчивый и быстрый базовый выбор
        KDK,         // Kick-Drift-Kick: симплектическая схема, удобна для поэтапного обновления сил
        RK4,         // Runge-Kutta 4-го порядка: высокая точность на шаг, но дороже по вычислениям
        Langevin,    // стохастический интегратор с термостатом (трение + случайный шум)
    };

    Integrator();

    void setScheme(Scheme scheme);
    Scheme getScheme() const { return integrator_type; }

    void step(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;

private:
    using SchemeVariant = std::variant<VerletScheme, KDKScheme, RK4Scheme, LangevinScheme>;

    static SchemeVariant makeSchemeImpl(Scheme scheme);

    Scheme integrator_type = Scheme::Verlet;
    SchemeVariant scheme_impl;
};
