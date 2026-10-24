#pragma once

#include <algorithm>
#include <span>

#include <Lattice/Tools/Exception.hpp>

#include "Laws.hpp"
#include "Program.hpp"

namespace Boundaries {

struct MaterialTable;

/// Check a material table against a program (every backend uses this one check):
/// at least the default material, every material index of the program exists, the
/// parameters of each law are valid. Returns the widest Soft layer — particles farther
/// from every surface feel no wall force.
inline float checkMaterials(const Program& program, std::span<const Material> materials) {
    using Error = Lattice::Exception<MaterialTable>;
    if (materials.empty())
        throw Error("walls need at least the default material");
    for (const auto& instruction : program.code)
        if (isPrimitive(instruction.op) && instruction.material >= materials.size())
            throw Error("geometry uses material {}, only {} defined", instruction.material, materials.size());
    float forceRange = 0.0f;
    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& material = materials[i];
        switch (material.law) {
            case Laws::Open::id:
                break;
            case Laws::Hard::id:
                if (material.params[0] < 0.0f || material.params[0] > 1.0f)
                    throw Error("material {}: restitution {} is outside [0, 1]", i, material.params[0]);
                break;
            case Laws::Soft::id:
                if (material.params[0] <= 0.0f || material.params[1] <= 0.0f || material.params[2] < 1.0f)
                    throw Error("material {}: soft wall needs stiffness > 0, range > 0, exponent >= 1", i);
                forceRange = std::max(forceRange, Laws::Soft::range(material));
                break;
            default:
                throw Error("material {}: unknown law {}", i, material.law);
        }
    }
    return forceRange;
}

}
