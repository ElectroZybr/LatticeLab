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
    LatticeStructure structure = LatticeStructure::Bcc;
    bool fixed = false;
    uint32_t seed = 0;
};

/// Заполняет регион кристаллической решеткой.
/// Composition использует species + fraction:
/// - сумма fraction задает общую заполненность решетки
/// - вид внутри занятого site выбирается пропорционально нормализованным fraction
/// - molecules are not supported
int latticeFill(Lattice::Simulation& sim, const Lattice::Generators::Region& region, const std::vector<Compose>& composition,
                const LatticeFillOptions& options = {});

} // namespace Generators
