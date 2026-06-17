#include "RandomFill.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace Generators {
namespace {

void loadBaseMoleculesOnce(Lattice::Simulation& sim) {
    static bool loaded = false;
    if (loaded) {
        return;
    }

    const std::filesystem::path moleculesPath = std::filesystem::path("Mods") / "Base" / "Molecules";
    if (!std::filesystem::exists(moleculesPath) || !std::filesystem::is_directory(moleculesPath)) {
        loaded = true;
        return;
    }

    std::vector<std::filesystem::path> pdbFiles;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(moleculesPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".pdb") {
            continue;
        }
        pdbFiles.push_back(entry.path());
    }

    std::sort(pdbFiles.begin(), pdbFiles.end());
    for (const std::filesystem::path& pdbPath : pdbFiles) {
        const std::string moleculeName = pdbPath.stem().string();
        if (!moleculeName.empty()) {
            sim.loadMoleculeTemplate(moleculeName, pdbPath);
        }
    }

    loaded = true;
}

} // namespace

int randomFill(Lattice::Simulation& sim, const Lattice::Generators::Region& region, const std::vector<Compose>& composition,
               const RandomFillOptions& options) {
    if (composition.empty() || options.density <= 0.0f) {
        return 0;
    }

    loadBaseMoleculesOnce(sim);
    const size_t initialAtomCount = sim.atoms().size();

    const Lattice::Generators::Bounds bounds = region.bounds();
    const glm::vec3 span = bounds.max - bounds.min;
    const float boundsVolume = std::max(0.0f, span.x) * std::max(0.0f, span.y) * std::max(0.0f, span.z);
    const int totalAtomBudget = std::max(0, static_cast<int>(std::lround(boundsVolume * options.density)));
    if (totalAtomBudget <= 0) {
        return 0;
    }

    float totalFraction = 0.0f;
    for (const Compose& entry : composition) {
        if (!entry.species.empty() && entry.fraction > 0.0f) {
            totalFraction += entry.fraction;
        }
    }
    if (totalFraction <= 0.0f) {
        return 0;
    }

    std::vector<std::string> speciesQueue;
    size_t totalCount = 0;
    for (const Compose& entry : composition) {
        if (entry.species.empty() || entry.fraction <= 0.0f) {
            continue;
        }

        int atomsPerSpawn = 1;
        if (const Lattice::MoleculeTemplate* molecule = sim.findMoleculeTemplate(entry.species); molecule != nullptr) {
            atomsPerSpawn = std::max<int>(1, static_cast<int>(molecule->atoms.size()));
        }

        const float normalizedFraction = entry.fraction / totalFraction;
        const int spawnCount = std::max(0, static_cast<int>(std::lround((totalAtomBudget * normalizedFraction) / atomsPerSpawn)));
        totalCount += static_cast<size_t>(spawnCount);
    }
    speciesQueue.reserve(totalCount);

    for (const Compose& entry : composition) {
        if (entry.species.empty() || entry.fraction <= 0.0f) {
            continue;
        }

        int atomsPerSpawn = 1;
        if (const Lattice::MoleculeTemplate* molecule = sim.findMoleculeTemplate(entry.species); molecule != nullptr) {
            atomsPerSpawn = std::max<int>(1, static_cast<int>(molecule->atoms.size()));
        }

        const float normalizedFraction = entry.fraction / totalFraction;
        const int spawnCount = std::max(0, static_cast<int>(std::lround((totalAtomBudget * normalizedFraction) / atomsPerSpawn)));
        for (int i = 0; i < spawnCount; ++i) {
            speciesQueue.push_back(entry.species);
        }
    }

    if (speciesQueue.empty()) {
        return 0;
    }

    std::mt19937 rng(options.seed == 0 ? std::random_device{}() : options.seed);
    std::shuffle(speciesQueue.begin(), speciesQueue.end(), rng);
    sim.beginAtomBatch();
    Lattice::SpawnOptions spawnOptions;
    spawnOptions.min = bounds.min;
    spawnOptions.max = bounds.max;
    spawnOptions.temperature = options.temperature;
    spawnOptions.maxAttempts = std::max<uint32_t>(1, options.maxAttemptsPerSpawn);
    spawnOptions.randomRotation = options.randomRotation;
    spawnOptions.fixed = options.fixed;

    int spawned = 0;
    for (const std::string& species : speciesQueue) {
        if (sim.randomSpawn(species, spawnOptions)) {
            ++spawned;
        }
    }
    sim.finishAtomBatch();

    for (size_t atomIndex = sim.atoms().size(); atomIndex > initialAtomCount; --atomIndex) {
        const size_t currentIndex = atomIndex - 1;
        if (!region.contains(sim.atoms().pos(currentIndex))) {
            sim.removeAtom(currentIndex);
        }
    }

    return static_cast<int>(sim.atoms().size() - initialAtomCount);
}

} // namespace Generators
