#include "PickingSystem.h"
#include "Engine/math/Ray.h"

#include <limits>

PickingSystem::PickingSystem(Camera& camera, AtomStorage& atomStorage)
    : camera(camera), atomStorage(atomStorage)
{}

void PickingSystem::clearSelection() {
    overlay.reset();
    std::fill_n(atomStorage.selectedData(), atomStorage.size(), 0);
}

void PickingSystem::processRect(sf::Vector2i start, sf::Vector2i end, bool cumulative) {
    if (!cumulative) selectedIndices.clear();

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        sf::Vector2i screenPos = camera.worldToScreen(atomStorage.pos(i));
        if (pointInRect(screenPos, start, end)) {
            selectedIndices.insert(i);
        }
    }
}

void PickingSystem::processLasso(std::span<sf::Vector2i> points, bool cumulative) {
    if (points.size() < 3) return;
    if (!cumulative) selectedIndices.clear();

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        sf::Vector2i screenPos = camera.worldToScreen(atomStorage.pos(i));
        if (pointInPolygon(screenPos, points)) {
            selectedIndices.insert(i);
        }
    }
}

bool PickingSystem::pickAtom2D(sf::Vector2i screenPos, float tolerance, AtomHit& hit) const {
    float bestDistSqr = tolerance*tolerance;
    std::size_t bestIndex = static_cast<std::size_t>(-1);

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const sf::Vector2i atomScreen = camera.worldToScreen(atomStorage.pos(i));
        const float distSqr = (atomScreen - screenPos).lengthSquared();
        if (distSqr < bestDistSqr) {
            bestDistSqr  = distSqr;
            bestIndex = i;
        }
    }

    if (bestIndex == static_cast<std::size_t>(-1))
        return false;

    hit = { bestIndex, std::sqrt(bestDistSqr) };
    return true;
}

// 3D: ray cast — ищем ближайший атом вдоль луча
bool PickingSystem::pickAtom3D(sf::Vector2i screenPos, AtomHit& hit) const {
    const Ray ray = camera.screenToRay(
        static_cast<float>(screenPos.x),
        static_cast<float>(screenPos.y)
    );

    float bestT = std::numeric_limits<float>::max();
    std::size_t bestIndex = static_cast<std::size_t>(-1);

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const Vec3f    pos    = atomStorage.pos(i);
        const Vec3f    center(pos.x, pos.y, pos.z);
        const float   radius = AtomData::getProps(atomStorage.type(i)).radius;

        RaySphereHit rayHit;
        if (raySphereIntersect(ray, center, radius, rayHit)) {
            if (rayHit.t < bestT) {
                bestT     = rayHit.t;
                bestIndex = i;
            }
        }
    }

    if (bestIndex == static_cast<std::size_t>(-1))
        return false;

    hit = { bestIndex, bestT };
    return true;
}

// Ray casting алгоритм для point-in-polygon
bool PickingSystem::pointInPolygon(sf::Vector2i point, std::span<sf::Vector2i> polygon)
{
    bool inside = false;
    const int x = point.x;
    const int y = point.y;
    const std::size_t n = polygon.size();

    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const int xi = polygon[i].x, yi = polygon[i].y;
        const int xj = polygon[j].x, yj = polygon[j].y;

        const bool intersects = ((yi > y) != (yj > y)) &&
                                (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
        if (intersects)
            inside = !inside;
    }

    return inside;
}

bool PickingSystem::pointInRect(sf::Vector2i point, sf::Vector2i start, sf::Vector2i end) {
    int minX = std::min(start.x, end.x);
    int maxX = std::max(start.x, end.x);
    int minY = std::min(start.y, end.y);
    int maxY = std::max(start.y, end.y);
    return (point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY);
}
