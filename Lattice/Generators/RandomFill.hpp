#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Generators/Region.hpp"

namespace Generators {

struct RandomFillOptions {
    float density = 0.0f;
    float temperature = 0.0f;
    uint32_t maxAttemptsPerSpawn = 32;
    bool randomRotation = true;
    bool fixed = false;
    uint32_t seed = 0;
};

struct Compose {
    std::string species;
    float fraction = 0.0f;
};

/// Случайно заполняет регион смесью атомов и молекул.
/// Каждая запись composition задает species name и долю.
/// Общее число спавнов выводится из density и bounds региона.
int randomFill(Lattice::Simulation& sim, const Lattice::Generators::Region& region, const std::vector<Compose>& composition,
               const RandomFillOptions& options = {});

} // namespace Generators
