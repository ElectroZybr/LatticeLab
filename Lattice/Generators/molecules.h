#pragma once

#include <glm/glm.hpp>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/physics/Atom/AtomData.h"

namespace Generators {
    /// Создает одну молекулу воды H2O в XY-плоскости.
    /// @param sim Симуляция
    /// @param oxygenPos Позиция атома кислорода
    /// @param velocity Начальная скорость всех атомов молекулы
    /// @param fixed Зафиксировать ли атомы молекулы
    void h2o(Lattice::Simulation& sim, glm::vec3 oxygenPos = glm::vec3(0.0f), glm::vec3 velocity = glm::vec3(0.0f), bool fixed = false);
}
