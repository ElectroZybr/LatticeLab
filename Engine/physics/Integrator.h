#pragma once

#include <vector>

class Atom;
class SpatialGrid;

class Integrator {
public:
    enum class Scheme {
        Verlet,      // классический verlet
        KDK,         // 
        RK4,
        Langevin,
    };

    using StepFn = void (Integrator::*)(Atom& atom, double dt) const;

    void setScheme(Scheme scheme) { integrator_type = scheme; }
    Scheme getScheme() const { return integrator_type; }
    void setGrid(SpatialGrid* grid_ptr) { grid = grid_ptr; }

    void predict(std::vector<Atom>& atoms, double dt) const;
    void correct(std::vector<Atom>& atoms, double dt) const;

private:
    Scheme integrator_type = Scheme::Verlet;
    SpatialGrid* grid = nullptr;

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
