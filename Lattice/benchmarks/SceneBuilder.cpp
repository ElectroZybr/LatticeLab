#include "SceneBuilder.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Generators/LatticeFill.hpp"
#include "Lattice/Generators/RandomFill.hpp"
#include "Lattice/Generators/Region.hpp"

namespace Benchmarks {
    namespace {
        constexpr float kOuterMargin = 8.0f;
        constexpr float kGasDensity = 0.01f;
    }

    void Scenes::build(Lattice::Simulation& simulation, SceneKind scene, int atomCount) {
        simulation.clear();

        switch (scene) {
        case SceneKind::Gas:
            buildGas(simulation, atomCount);
            break;
        case SceneKind::Crystal:
            buildCrystal(simulation, atomCount);
            break;
        }
    }

    int cubeSideFromCount(int atomCount) { return std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<double>(atomCount))))); }

    void Scenes::buildGas(Lattice::Simulation& simulation, int atomCount) {
        const AtomData::Type atomType = AtomData::Type::Z;
        const float volume = static_cast<float>(std::max(atomCount, 1)) / kGasDensity;
        const float side = std::cbrt(volume);
        const glm::vec3 worldSize(side + kOuterMargin * 2.0f);

        simulation.setSizeBox(worldSize);

        Generators::Rectangle region;
        region.center = worldSize * 0.5f;
        region.boxSize = glm::vec3(side);

        Generators::RandomFillOptions options;
        options.mode = Generators::SpawnMode::Reset;
        options.density = kGasDensity;
        options.temperature = 0.0f;
        options.margin = 0.0f;
        options.randomRotation = false;
        options.fixed = false;
        options.seed = 1;

        Generators::randomFill(simulation, region, {{std::string(AtomData::symbol(atomType)), 1.0f}}, options);
    }

    void Scenes::buildCrystal(Lattice::Simulation& simulation, int atomCount) {
        const int side = cubeSideFromCount(atomCount);
        const AtomData::Type atomType = AtomData::Type::Z;
        const float spacing = simulation.lj_min(atomType, atomType);
        const glm::vec3 crystalSize(static_cast<float>(side) * spacing);

        simulation.setSizeBox(crystalSize + glm::vec3(kOuterMargin * 2.0f));

        Generators::Rectangle region;
        region.center = simulation.world().getWorldSize() * 0.5f;
        region.boxSize = crystalSize;

        Generators::LatticeFillOptions options;
        options.mode = Generators::SpawnMode::Reset;
        options.structure = Generators::LatticeStructure::Hex;
        options.seed = 1;

        Generators::latticeFill(simulation, region, {{std::string(AtomData::symbol(atomType)), 1.0f}}, options);
    }
}
