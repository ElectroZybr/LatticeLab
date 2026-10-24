#pragma once

#include <cstddef>

#include <Boundaries/ParticleLayout.hpp>
#include <Boundaries/Space.hpp>
#include <StdData/SoA.hpp>

namespace Boundaries::Cpu {

/// Wrap the positions of rows [0, rows) into the box along periodic axes: one pass per
/// periodic axis over its column.
void wrapPositions(const Space& space, StdData::SoA& particles, const ParticleLayout& layout, size_t rows);

}
