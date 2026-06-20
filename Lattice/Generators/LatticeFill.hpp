#pragma once

#include <cstdint>
#include <vector>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Generators/RandomFill.hpp"
#include "Lattice/Generators/Region.hpp"

namespace Generators {

enum class LatticeStructure : uint8_t {
    Bcc,
    Hex,
};

struct LatticeFillOptions {
    SpawnMode mode = SpawnMode::Replace;
    LatticeStructure structure = LatticeStructure::Hex;
    float margin = 0.0f;
    bool fixed = false;
    uint32_t seed = 0;
};

/// Заполняет регион кристаллической решеткой.
/// Composition использует species + fraction:
/// - сумма fraction задает общую заполненность решетки
/// - вид внутри занятого site выбирается пропорционально нормализованным fraction
/// - molecules are not supported
int latticeFill(Lattice::Simulation& sim, const Generators::Region& region, const std::vector<Compose>& composition,
                const LatticeFillOptions& options = {});

} // namespace Generators
