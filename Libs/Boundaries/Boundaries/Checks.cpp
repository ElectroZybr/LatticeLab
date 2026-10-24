#include "Checks.hpp"

#include <cmath>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries {

namespace {
struct PeriodicSeam;
using Error = Lattice::Exception<PeriodicSeam>;
}

void checkPeriodicSeams(const Space& space, const Geometry& geometry, int samples, float tolerance) {
    const glm::vec3 origin = space.origin(), size = space.size();
    for (int axis = 0; axis < 3; ++axis) {
        if (!space.periodic()[axis])
            continue;
        const int u = (axis + 1) % 3, v = (axis + 2) % 3;
        for (int i = 0; i < samples; ++i)
            for (int j = 0; j < samples; ++j) {
                glm::vec3 low = origin;
                low[u] += size[u] * (float(i) + 0.5f) / float(samples);
                low[v] += size[v] * (float(j) + 0.5f) / float(samples);
                glm::vec3 high = low;
                high[axis] += size[axis];
                const float a = geometry.distance(low), b = geometry.distance(high);
                if (std::abs(a - b) > tolerance)
                    throw Error("geometry does not match across the periodic seam of axis {}: "
                        "distance {} at ({}, {}, {}) and {} on the opposite face", axis, a, low.x, low.y, low.z, b);
            }
    }
}

}
