#pragma once

#include "Geometry.hpp"
#include "Space.hpp"

namespace Boundaries {

/// Along a periodic axis the geometry must match itself across the seam: distances on
/// the two opposite faces of the box are compared on a grid of `samples` × `samples`
/// points. A mismatch is a configuration error (the region would tear at the seam).
void checkPeriodicSeams(const Space& space, const Geometry& geometry, int samples = 16, float tolerance = 1e-3f);

}
