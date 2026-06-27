#pragma once

#include <filesystem>

namespace Lattice {
    class Simulation;
}

namespace MoleculeTemplatesIO {
void loadFromDirectory(Lattice::Simulation& simulation, const std::filesystem::path& directory);
}
