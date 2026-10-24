#pragma once

#include <cmath>
#include <cstddef>

#include <Boundaries/ParticleLayout.hpp>
#include <Boundaries/Space.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <StdData/SoA.hpp>

namespace Boundaries::Cpu {

/// Wrap the positions of rows [0, rows) into the box along periodic axes: one pass per
/// periodic axis over its column.
inline void wrapPositions(const Space& space, StdData::SoA& particles, const ParticleLayout& layout, size_t rows) {
    if (rows > particles.size())
        throw Lattice::Exception<Space>("wrap: {} rows of a table with {}", rows, particles.size());
    const std::string* names[3] = {&layout.x, &layout.y, &layout.z};
    const glm::vec3 origin = space.origin(), size = space.size(), inverse = space.inverseSize();
    for (int axis = 0; axis < 3; ++axis) {
        if (!space.periodic()[axis])
            continue;
        const float o = origin[axis], l = size[axis], k = inverse[axis];
        for (float& value : particles.span<float>(*names[axis]).first(rows)) {
            float wrapped = (value - o) - l * std::floor((value - o) * k);
            if (wrapped >= l)
                wrapped -= l;
            value = o + wrapped;
        }
    }
}

}
