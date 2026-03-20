#include "Integrator.h"

#include <cassert>

#include "Atom.h"

Integrator::Integrator() {
    setScheme(Scheme::Verlet);
}

void Integrator::setScheme(Scheme scheme) {
    integrator_type = scheme;
    predict_step = selectPredictStep(scheme);
    correct_step = selectCorrectStep(scheme);
}

void Integrator::predict(std::vector<Atom>& atoms, double dt) const {
    for (Atom& atom : atoms) {
        int prev_x = grid->worldToCellX(atom.coords.x);
        int prev_y = grid->worldToCellY(atom.coords.y);
        int prev_z = grid->worldToCellZ(atom.coords.z);

        if (atom.isFixed == false)
            (this->*predict_step)(atom, dt);

        int curr_x = grid->worldToCellX(atom.coords.x);
        int curr_y = grid->worldToCellY(atom.coords.y);
        int curr_z = grid->worldToCellZ(atom.coords.z);
        if (prev_x != curr_x || prev_y != curr_y || prev_z != curr_z) {
            grid->erase(prev_x, prev_y, prev_z, &atom);
            grid->insert(curr_x, curr_y, curr_z, &atom);
        }

        atom.prev_force = atom.force;
        atom.force = Vec3D(0, 0, 0);
        atom.potential_energy = 0.0;
    }
}

void Integrator::correct(std::vector<Atom>& atoms, double dt) const {
    for (Atom& atom : atoms) {
        (this->*correct_step)(atom, dt);
    }
}

Integrator::StepFn Integrator::selectPredictStep(Scheme scheme) const {
    switch (scheme) {
    case Scheme::Verlet:
        return &Integrator::verletPredict;
    case Scheme::KDK:
        return &Integrator::kdkPredict;
    case Scheme::RK4:
        return &Integrator::rk4Predict;
    case Scheme::Langevin:
        return &Integrator::langevinPredict;
    default:
        return &Integrator::verletPredict;
    }
}

Integrator::StepFn Integrator::selectCorrectStep(Scheme scheme) const {
    switch (scheme) {
    case Scheme::Verlet:
        return &Integrator::verletCorrect;
    case Scheme::KDK:
        return &Integrator::kdkCorrect;
    case Scheme::RK4:
        return &Integrator::rk4Correct;
    case Scheme::Langevin:
        return &Integrator::langevinCorrect;
    default:
        return &Integrator::verletCorrect;
    }
}

void Integrator::verletPredict(Atom& atom, double dt) const {
    /* Предсказание новой позиции на основе предыдущей и ускорения */
    constexpr float dempfer = 1.f; // демпфирование для устойчивости
    Vec3D a = atom.force / atom.getProps().mass;
    atom.coords += (atom.speed * dempfer + a * 0.5f * dt) * dt;
}

void Integrator::verletCorrect(Atom& atom, double dt) const {
    /* Обновление скорости с использованием среднего ускорения */
    Vec3D a = atom.force / atom.getProps().mass;
    Vec3D pr_a = atom.prev_force / atom.getProps().mass;
    atom.speed += (pr_a + a) * 0.5f * dt;
}

void Integrator::kdkPredict(Atom& atom, double dt) const {
    // TODO: add dedicated KDK stage logic.
    verletPredict(atom, dt);
}

void Integrator::kdkCorrect(Atom& atom, double dt) const {
    // TODO: add dedicated KDK stage logic.
    verletCorrect(atom, dt);
}

void Integrator::rk4Predict(Atom& atom, double dt) const {
    // TODO: add dedicated RK4 stage logic.
}

void Integrator::rk4Correct(Atom& atom, double dt) const {
    // TODO: add dedicated RK4 stage logic.
    verletCorrect(atom, dt);
}

void Integrator::langevinPredict(Atom& atom, double dt) const {
    // TODO: add dedicated Langevin stage logic.
    verletPredict(atom, dt);
}

void Integrator::langevinCorrect(Atom& atom, double dt) const {
    // TODO: add dedicated Langevin stage logic.
    verletCorrect(atom, dt);
}
