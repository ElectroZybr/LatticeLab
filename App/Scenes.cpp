#include "Scenes.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <vector>

namespace Scenes {
namespace detail {
    bool hasNeighborInStorage(const Simulation& sim, const Vec3f& coords, float delta) {
        const int cx = sim.sim_box.grid.worldToCellX(coords.x);
        const int cy = sim.sim_box.grid.worldToCellY(coords.y);
        const int cz = sim.sim_box.grid.worldToCellZ(coords.z);
        const float deltaSqr = delta * delta;
        const int radiusCells = std::max(1, static_cast<int>(std::ceil(delta / static_cast<float>(sim.sim_box.grid.cellSize))));

        for (int dz = -radiusCells; dz <= radiusCells; ++dz) {
            for (int dy = -radiusCells; dy <= radiusCells; ++dy) {
                for (int dx = -radiusCells; dx <= radiusCells; ++dx) {
                    const int nx = cx + dx;
                    const int ny = cy + dy;
                    const int nz = cz + dz;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= sim.sim_box.grid.sizeX || ny >= sim.sim_box.grid.sizeY || nz >= sim.sim_box.grid.sizeZ) {
                        continue;
                    }

                    auto cell = sim.sim_box.grid.atomsInCell(nx, ny, nz);
                    for (size_t atomIndex : cell) {
                        if (atomIndex >= sim.atomStorage.size()) {
                            continue;
                        }
                        if ((coords - sim.atomStorage.pos(atomIndex)).sqrAbs() < deltaSqr) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    uint32_t resolveSeed(uint32_t seed) {
        return seed == 0 ? std::random_device{}() : seed;
    }
} // namespace detail

void crystal(Simulation& sim, int n, AtomData::Type type, bool is3d, double padding, double margin) {
    const double side = n * padding + padding + 2.0 * margin;

    sim.setSizeBox(
        Vec3f(side, side, is3d ? side : sim.sim_box.size.z)
    );

    const Vec3f vecMargin(margin, margin, is3d ? margin : 0.0);
    const int zMax = is3d ? n : 1;
    const size_t atomTotal = static_cast<size_t>(n)
        * static_cast<size_t>(n)
        * static_cast<size_t>(zMax);
    sim.atomStorage.reserve(sim.atomStorage.size() + atomTotal);

    for (int x = 1; x <= n; ++x) {
        for (int y = 1; y <= n; ++y) {
            for (int z = 1; z <= zMax; ++z) {
                sim.atomStorage.addAtom(
                    Vec3f(x, y, z) * padding + vecMargin,
                    Vec3f::Random() * 0.5f,
                    type
                );
            }
        }
    }

    sim.sim_box.grid.rebuild(
        sim.atomStorage.xDataSpan(),
        sim.atomStorage.yDataSpan(),
        sim.atomStorage.zDataSpan()
    );
    sim.neighborList.clear();
}

int randomGasInCurrentBox(Simulation& sim,
                          int atomCount,
                          AtomData::Type type,
                          bool is3d,
                          float minDistance,
                          float speedScale,
                          int maxAttemptsPerAtom,
                          uint32_t seed) {
    atomCount = std::max(0, atomCount);
    if (atomCount == 0) {
        sim.neighborList.clear();
        return 0;
    }

    std::srand(static_cast<unsigned>(detail::resolveSeed(seed)));

    const float minDistanceSqr = minDistance * minDistance;

    const size_t oldSize = sim.atomStorage.size();
    std::vector<Vec3f> acceptedPositions;
    acceptedPositions.reserve(static_cast<size_t>(atomCount));

    std::vector<std::vector<Vec3f>> pendingByCell(static_cast<size_t>(sim.sim_box.grid.countCells));
    const int pendingRadiusCells = std::max(1, static_cast<int>(std::ceil(minDistance / static_cast<float>(sim.sim_box.grid.cellSize))));

    const auto isTooCloseToPending = [&](const Vec3f& coords) {
        const int cx = sim.sim_box.grid.worldToCellX(coords.x);
        const int cy = sim.sim_box.grid.worldToCellY(coords.y);
        const int cz = sim.sim_box.grid.worldToCellZ(coords.z);

        for (int dz = -pendingRadiusCells; dz <= pendingRadiusCells; ++dz) {
            for (int dy = -pendingRadiusCells; dy <= pendingRadiusCells; ++dy) {
                for (int dx = -pendingRadiusCells; dx <= pendingRadiusCells; ++dx) {
                    const int nx = cx + dx;
                    const int ny = cy + dy;
                    const int nz = cz + dz;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= sim.sim_box.grid.sizeX || ny >= sim.sim_box.grid.sizeY || nz >= sim.sim_box.grid.sizeZ) {
                        continue;
                    }

                    const int cellIndex = sim.sim_box.grid.linearIndex(nx, ny, nz);
                    const auto& bucket = pendingByCell[static_cast<size_t>(cellIndex)];
                    for (const Vec3f& other : bucket) {
                        if ((coords - other).sqrAbs() < minDistanceSqr) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    };

    const double zMid = sim.sim_box.size.z * 0.5;
    const double zSpan = sim.sim_box.size.z - 4.0;
    const int maxZ = std::max(0, static_cast<int>(zSpan));
    for (int i = 0; i < atomCount; ++i) {
        for (int attempt = 0; attempt < maxAttemptsPerAtom; ++attempt) {
            const double rx = std::rand() % int(sim.sim_box.size.x - 4.0);
            const double ry = std::rand() % int(sim.sim_box.size.y - 4.0);
            const double rz = is3d ? (std::rand() % (maxZ + 1)) : zMid;
            const Vec3f coords(rx + 2.0, ry + 2.0, is3d ? (rz + 2.0) : zMid);

            if (!detail::hasNeighborInStorage(sim, coords, minDistance) && !isTooCloseToPending(coords)) {
                acceptedPositions.emplace_back(coords);
                const int cell = sim.sim_box.grid.linearIndex(
                    sim.sim_box.grid.worldToCellX(coords.x),
                    sim.sim_box.grid.worldToCellY(coords.y),
                    sim.sim_box.grid.worldToCellZ(coords.z)
                );
                pendingByCell[static_cast<size_t>(cell)].emplace_back(coords);
                break;
            }
        }
    }

    if (acceptedPositions.empty()) {
        sim.neighborList.clear();
        return 0;
    }

    sim.atomStorage.reserve(oldSize + acceptedPositions.size());
    for (const Vec3f& pos : acceptedPositions) {
        const Vec3f randomSpeed = Vec3f::Random() * speedScale;
        sim.atomStorage.addAtom(pos, is3d ? randomSpeed : Vec3f(randomSpeed.x, randomSpeed.y, 0.0f), type);
    }

    sim.sim_box.grid.rebuild(
        sim.atomStorage.xDataSpan(),
        sim.atomStorage.yDataSpan(),
        sim.atomStorage.zDataSpan()
    );
    sim.neighborList.clear();
    return static_cast<int>(acceptedPositions.size());
}

void randomGas(Simulation& sim,
               int atomCount,
               AtomData::Type type,
               bool is3d,
               double spacing,
               double margin,
               float density,
               float speedScale,
               uint32_t seed) {
    atomCount = std::max(0, atomCount);
    const float clampedDensity = std::clamp(density, 0.25f, 3.0f);
    const double effectiveSpacing = spacing / static_cast<double>(clampedDensity);

    const int sideCount = is3d
        ? std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<double>(atomCount)))))
        : std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(atomCount)))));

    const double span = sideCount * effectiveSpacing + 2.0 * margin;

    sim.setSizeBox(
        Vec3f(span, span, is3d ? span : 6)
    );

    randomGasInCurrentBox(sim, atomCount, type, is3d, 4.0f, speedScale, 20, seed);
}

} // namespace Scenes
