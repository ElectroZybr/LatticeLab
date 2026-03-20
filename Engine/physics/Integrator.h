#pragma once

#include <vector>

class Atom;
class SpatialGrid;

class Integrator {
public:
    enum class Scheme {
        Verlet,      // классический Velocity Verlet: устойчивый и быстрый базовый выбор
        KDK,         // Kick-Drift-Kick: симплектическая схема, удобна для поэтапного обновления сил
        RK4,         // Runge-Kutta 4-го порядка: высокая точность на шаг, но дороже по вычислениям
        Langevin,    // стохастический интегратор с термостатом (трение + случайный шум)
    };

    using StepFn = void (Integrator::*)(Atom& atom, double dt) const;

    Integrator();

    void setScheme(Scheme scheme);
    Scheme getScheme() const { return integrator_type; }
    void setGrid(SpatialGrid* grid_ptr) { grid = grid_ptr; }

    void predict(std::vector<Atom>& atoms, double dt) const;
    void correct(std::vector<Atom>& atoms, double dt) const;

private:
    Scheme integrator_type = Scheme::Verlet;
    SpatialGrid* grid = nullptr;
    StepFn predict_step = nullptr;
    StepFn correct_step = nullptr;

    StepFn selectPredictStep(Scheme scheme) const;
    StepFn selectCorrectStep(Scheme scheme) const;

    void verletPredict(Atom& atom, double dt) const;
    void verletCorrect(Atom& atom, double dt) const;

    void kdkPredict(Atom& atom, double dt) const;
    void kdkCorrect(Atom& atom, double dt) const;

    void rk4Predict(Atom& atom, double dt) const;
    void rk4Correct(Atom& atom, double dt) const;

    void langevinPredict(Atom& atom, double dt) const;
    void langevinCorrect(Atom& atom, double dt) const;
};
