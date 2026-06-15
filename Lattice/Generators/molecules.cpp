#include "molecules.h"

#include <glm/trigonometric.hpp>

namespace Generators {
    void h2o(Lattice::Simulation& sim, glm::vec3 oxygenPos, glm::vec3 velocity, bool fixed) {
        constexpr float kOHBondLength = 0.9572f;
        constexpr float kHOHAngleDegrees = 104.5f;
        const float halfAngle = glm::radians(kHOHAngleDegrees * 0.5f);

        const glm::vec3 hydrogenOffsetA(glm::cos(halfAngle) * kOHBondLength, glm::sin(halfAngle) * kOHBondLength, 0.0f);
        const glm::vec3 hydrogenOffsetB(glm::cos(halfAngle) * kOHBondLength, -glm::sin(halfAngle) * kOHBondLength, 0.0f);

        const size_t oxygenIndex = sim.atoms().size();
        sim.appendAtomFast(oxygenPos, velocity, AtomData::Type::O, fixed);
        sim.appendAtomFast(oxygenPos + hydrogenOffsetA, velocity, AtomData::Type::H, fixed);
        sim.appendAtomFast(oxygenPos + hydrogenOffsetB, velocity, AtomData::Type::H, fixed);
        sim.finalizeAtomBatch();

        sim.addBond(oxygenIndex, oxygenIndex + 1);
        sim.addBond(oxygenIndex, oxygenIndex + 2);
    }
}
