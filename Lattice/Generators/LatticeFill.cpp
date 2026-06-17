#include "LatticeFill.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <random>
#include <string>

namespace Generators {
namespace {

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

bool spawnSite(Lattice::Simulation& sim, glm::vec3 position, const std::vector<Compose>& composition, float totalShare,
               bool fixed, std::mt19937& rng) {
    std::uniform_real_distribution<float> occupancyDist(0.0f, 1.0f);
    if (occupancyDist(rng) > totalShare) {
        return false;
    }

    const std::string species = chooseSpecies(composition, totalShare, rng);
    return sim.spawnMolecule(species, position, std::nullopt, fixed);
}

} // namespace

int latticeFill(Lattice::Simulation& sim, const Lattice::Generators::Region& region, const std::vector<Compose>& composition,
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
    const Lattice::Generators::Bounds bounds = region.bounds();
    const glm::vec3 size = bounds.max - bounds.min;

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

                    if (region.contains(origin) && spawnSite(sim, origin, composition, shareTotal, options.fixed, rng)) {
                        ++spawned;
                    }
                    if (region.contains(center) && spawnSite(sim, center, composition, shareTotal, options.fixed, rng)) {
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
                    if (region.contains(position) && spawnSite(sim, position, composition, shareTotal, options.fixed, rng)) {
                        ++spawned;
                    }
                }
            }
        }
    }

    sim.finishAtomBatch();
    return spawned;
}

} // namespace Generators
