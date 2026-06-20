#include "LatticeFill.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <random>
#include <string>
#include <unordered_set>

namespace Generators {
namespace {

Generators::Bounds clampBoundsToWorld(const Generators::Bounds& bounds, glm::vec3 worldSize, float margin) {
    const glm::vec3 safeMargin = glm::vec3(std::max(margin, 0.0f));
    const glm::vec3 worldMin = glm::min(safeMargin, worldSize * 0.5f);
    const glm::vec3 worldMax = glm::max(worldSize - safeMargin, worldMin);

    return {
        .min = glm::clamp(bounds.min, worldMin, worldMax),
        .max = glm::clamp(bounds.max, worldMin, worldMax),
    };
}

bool containsWithMargin(const Generators::Region& region, glm::vec3 point, float margin) {
    if (!region.contains(point)) {
        return false;
    }

    if (margin <= 0.0f) {
        return true;
    }

    return region.contains(point + glm::vec3( margin, 0.0f, 0.0f)) &&
           region.contains(point + glm::vec3(-margin, 0.0f, 0.0f)) &&
           region.contains(point + glm::vec3(0.0f,  margin, 0.0f)) &&
           region.contains(point + glm::vec3(0.0f, -margin, 0.0f)) &&
           region.contains(point + glm::vec3(0.0f, 0.0f,  margin)) &&
           region.contains(point + glm::vec3(0.0f, 0.0f, -margin));
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string normalizeAtomSymbol(std::string value) {
    value = lowercase(std::move(value));
    if (!value.empty()) {
        value[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(value[0])));
    }
    return value;
}

std::optional<AtomData::Type> findAtomTypeBySymbol(std::string_view speciesName) {
    const std::string atomSymbol = normalizeAtomSymbol(std::string(speciesName));
    for (size_t i = 0; i < static_cast<size_t>(AtomData::Type::COUNT); ++i) {
        const AtomData::Type type = static_cast<AtomData::Type>(i);
        if (AtomData::symbol(type) == atomSymbol) {
            return type;
        }
    }
    return std::nullopt;
}

float totalFraction(const std::vector<Compose>& composition) {
    float total = 0.0f;
    for (const Compose& entry : composition) {
        if (!entry.species.empty() && entry.fraction > 0.0f) {
            total += entry.fraction;
        }
    }
    return total;
}

const Compose* dominantEntry(const std::vector<Compose>& composition) {
    const Compose* dominant = nullptr;
    for (const Compose& entry : composition) {
        if (entry.species.empty() || entry.fraction <= 0.0f) {
            continue;
        }
        if (dominant == nullptr || entry.fraction > dominant->fraction) {
            dominant = &entry;
        }
    }
    return dominant;
}

bool isAtomicComposition(const std::vector<Compose>& composition) {
    for (const Compose& entry : composition) {
        if (entry.species.empty() || entry.fraction <= 0.0f) {
            continue;
        }
        if (!findAtomTypeBySymbol(entry.species).has_value()) {
            return false;
        }
    }
    return true;
}

std::string chooseSpecies(const std::vector<Compose>& composition, float totalShare, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, totalShare);
    const float threshold = dist(rng);
    float cumulative = 0.0f;

    for (const Compose& entry : composition) {
        if (entry.species.empty() || entry.fraction <= 0.0f) {
            continue;
        }
        cumulative += entry.fraction;
        if (threshold <= cumulative) {
            return entry.species;
        }
    }

    return composition.back().species;
}

void removeAtomIndicesDescending(Lattice::Simulation& sim, std::vector<size_t>& atomIndices) {
    if (atomIndices.empty()) {
        return;
    }

    std::sort(atomIndices.begin(), atomIndices.end());
    atomIndices.erase(std::unique(atomIndices.begin(), atomIndices.end()), atomIndices.end());
    sim.removeAtoms(std::move(atomIndices));
}

std::vector<size_t> collectCollidingAtoms(const Lattice::Simulation& sim, glm::vec3 position, float minDistance,
                                          const std::unordered_set<AtomStorage::AtomId>& initialAtomIds) {
    std::vector<size_t> collidingIndices;
    if (initialAtomIds.empty()) {
        return collidingIndices;
    }

    const World& world = sim.world();
    const AtomStorage& storage = sim.atoms();
    const SpatialGrid& grid = world.getGrid();
    const float minDistanceSqr = minDistance * minDistance;
    const int cellX = static_cast<int>(grid.worldToCellX(position.x));
    const int cellY = static_cast<int>(grid.worldToCellY(position.y));
    const int cellZ = static_cast<int>(grid.worldToCellZ(position.z));

    grid.forEachNeighborCell(cellX, cellY, cellZ, [&](int nx, int ny, int nz) {
        for (const uint32_t atomIndex : grid.atomsInCell(nx, ny, nz)) {
            if (!initialAtomIds.contains(storage.atomId(atomIndex))) {
                continue;
            }

            const glm::vec3 delta = position - storage.pos(atomIndex);
            if (glm::dot(delta, delta) < minDistanceSqr) {
                collidingIndices.push_back(atomIndex);
            }
        }
    });

    return collidingIndices;
}

bool spawnSite(Lattice::Simulation& sim, glm::vec3 position, const std::vector<Compose>& composition, float totalShare,
               float minDistance, const std::unordered_set<AtomStorage::AtomId>& initialAtomIds, SpawnMode mode, bool fixed, std::mt19937& rng) {
    std::uniform_real_distribution<float> occupancyDist(0.0f, 1.0f);
    if (occupancyDist(rng) > totalShare) {
        return false;
    }

    const std::string species = chooseSpecies(composition, totalShare, rng);
    std::vector<size_t> collidingIndices = collectCollidingAtoms(sim, position, minDistance, initialAtomIds);
    if (mode == SpawnMode::Replace) {
        removeAtomIndicesDescending(sim, collidingIndices);
    } else if (!collidingIndices.empty()) {
        return false;
    }

    return sim.spawnMolecule(species, position, std::nullopt, fixed);
}

} // namespace

int latticeFill(Lattice::Simulation& sim, const Generators::Region& region, const std::vector<Compose>& composition,
                const LatticeFillOptions& options) {
    if (composition.empty()) {
        return 0;
    }

    const float shareTotal = totalFraction(composition);
    if (shareTotal <= 0.0f || shareTotal > 1.0f) {
        return 0;
    }

    if (!isAtomicComposition(composition)) {
        return 0;
    }

    const Compose* dominant = dominantEntry(composition);
    if (dominant == nullptr) {
        return 0;
    }

    const std::optional<AtomData::Type> dominantType = findAtomTypeBySymbol(dominant->species);
    if (!dominantType.has_value()) {
        return 0;
    }

    const float spacing = sim.lj_min(*dominantType, *dominantType);
    if (spacing <= 0.0f) {
        return 0;
    }

    std::mt19937 rng(options.seed == 0 ? std::random_device{}() : options.seed);
    const float margin = std::max(options.margin, 0.5f * spacing);
    const Generators::Bounds bounds = clampBoundsToWorld(region.bounds(), sim.world().getWorldSize(), margin);
    const glm::vec3 size = bounds.max - bounds.min;
    const size_t initialAtomCount = sim.atoms().size();
    std::unordered_set<AtomStorage::AtomId> initialAtomIds;
    initialAtomIds.reserve(initialAtomCount);
    for (const AtomStorage::AtomId atomId : sim.atoms().atomIdDataSpan()) {
        initialAtomIds.insert(atomId);
    }

    int spawned = 0;
    sim.beginAtomBatch();

    if (options.structure == LatticeStructure::Bcc) {
        const int nx = std::max(0, static_cast<int>(std::floor(size.x / spacing)));
        const int ny = std::max(0, static_cast<int>(std::floor(size.y / spacing)));
        const int nz = std::max(0, static_cast<int>(std::floor(size.z / spacing)));

        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    const glm::vec3 origin(
                        bounds.min.x + x * spacing,
                        bounds.min.y + y * spacing,
                        bounds.min.z + z * spacing
                    );
                    const glm::vec3 center = origin + glm::vec3(0.5f * spacing);

                    if (region.contains(origin) &&
                        spawnSite(sim, origin, composition, shareTotal, spacing, initialAtomIds, options.mode, options.fixed, rng)) {
                        ++spawned;
                    }
                    if (region.contains(center) &&
                        spawnSite(sim, center, composition, shareTotal, spacing, initialAtomIds, options.mode, options.fixed, rng)) {
                        ++spawned;
                    }
                }
            }
        }
    }
    else if (options.structure == LatticeStructure::Hex) {
        const float rowStep = spacing * std::sqrt(3.0f) * 0.5f;
        const float layerShiftY = spacing * std::sqrt(3.0f) / 6.0f;
        const float layerStep = spacing * std::sqrt(2.0f / 3.0f);

        const int nz = std::max(0, static_cast<int>(std::floor(size.z / layerStep)) + 1);
        for (int z = 0; z < nz; ++z) {
            const bool isBLayer = (z % 2) == 1;
            const float zCoord = bounds.min.z + z * layerStep;
            if (zCoord > bounds.max.z) {
                break;
            }

            const int ny = std::max(0, static_cast<int>(std::floor(size.y / rowStep)) + 1);
            for (int y = 0; y < ny; ++y) {
                const bool oddRow = (y % 2) == 1;
                const float xOffset = (oddRow ? 0.5f * spacing : 0.0f) + (isBLayer ? 0.5f * spacing : 0.0f);
                const float yCoord = bounds.min.y + y * rowStep + (isBLayer ? layerShiftY : 0.0f);
                if (yCoord > bounds.max.y) {
                    break;
                }

                const int nx = std::max(0, static_cast<int>(std::floor((size.x - xOffset) / spacing)) + 1);
                for (int x = 0; x < nx; ++x) {
                    const float xCoord = bounds.min.x + x * spacing + xOffset;
                    if (xCoord > bounds.max.x) {
                        break;
                    }

                    const glm::vec3 position(xCoord, yCoord, zCoord);
                    if (region.contains(position) &&
                        spawnSite(sim, position, composition, shareTotal, spacing, initialAtomIds, options.mode, options.fixed, rng)) {
                        ++spawned;
                    }
                }
            }
        }
    }

    sim.finishAtomBatch();
    const glm::vec3 worldSize = sim.world().getWorldSize();
    std::vector<size_t> atomIndicesToRemove;
    for (size_t atomIndex = sim.atoms().size(); atomIndex > initialAtomCount; --atomIndex) {
        const size_t currentIndex = atomIndex - 1;
        const glm::vec3 position = sim.atoms().pos(currentIndex);
        const bool insideWorld =
            position.x >= margin && position.y >= margin && position.z >= margin &&
            position.x <= worldSize.x - margin &&
            position.y <= worldSize.y - margin &&
            position.z <= worldSize.z - margin;
        if (!insideWorld || !containsWithMargin(region, position, margin)) {
            atomIndicesToRemove.push_back(currentIndex);
        }
    }
    sim.removeAtoms(std::move(atomIndicesToRemove));

    return static_cast<int>(sim.atoms().size() - initialAtomCount);
}

} // namespace Generators
