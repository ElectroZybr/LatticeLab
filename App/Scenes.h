#pragma once

#include <cstdint>

#include "Engine/Simulation.h"
#include "Engine/physics/AtomData.h"

namespace Scenes {
    void crystal(Simulation& sim,
                 int n,
                 AtomData::Type type,
                 bool is3d,
                 double padding = 3.0,
                 double margin = 15.0);

    int randomGasInCurrentBox(Simulation& sim,
                              int atomCount,
                              AtomData::Type type,
                              bool is3d,
                              float minDistance = 4.0f,
                              float speedScale = 5.0f,
                              int maxAttemptsPerAtom = 20,
                              std::uint32_t seed = 0);

    void randomGas(Simulation& sim,
                   int atomCount,
                   AtomData::Type type,
                   bool is3d,
                   double spacing = 6.0,
                   double margin = 6.0,
                   float density = 1.0f,
                   float speedScale = 5.0f,
                   std::uint32_t seed = 0);
} // namespace Scenes
