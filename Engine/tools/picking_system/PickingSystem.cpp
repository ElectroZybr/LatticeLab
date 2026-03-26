#include "PickingSystem.h"
#include "Engine/math/Ray.h"
#include "Engine/SimBox.h"

#include <limits>

PickingSystem::PickingSystem(Camera& camera, AtomStorage& atomStorage, SimBox& box)
    : camera(camera), atomStorage(atomStorage), box(box)
{}

void PickingSystem::clearSelection() {
    overlay.reset();
    selectedIndices.clear();
    std::fill_n(atomStorage.selectedData(), atomStorage.size(), 0);
}

void PickingSystem::processClick(sf::Vector2i screenPos, bool cumulative) {
    const sf::Vector2f vSize = camera.getView().getSize();
    const sf::Vector2f vCenter = camera.getView().getCenter();
    // Предположим, у тебя есть метод получения размера экрана в камере
    // Если нет, просто выведи то, что доступно внутри camera.worldToScreen
    AtomHit hit;
    bool found = pickAtom(screenPos, 10.0f, hit);


    if (found) {
        if (!cumulative) clearSelection();

        // Если атом уже был выбран и зажат Ctrl — инвертируем (снимаем выделение)
        if (cumulative && selectedIndices.contains(hit.index)) {
            selectedIndices.erase(hit.index);
            atomStorage.setSelected(hit.index, false);
        } 
        else {
            // Иначе — добавляем в набор
            selectedIndices.insert(hit.index);
            atomStorage.setSelected(hit.index, true);
        }
    }
    else {
        // Клик в пустоту без Ctrl — сбрасываем всё
        if (!cumulative) clearSelection();
    }
}


void PickingSystem::processRect(sf::Vector2i start, sf::Vector2i end, bool cumulative) {
    if (!cumulative) clearSelection();

    const sf::Vector2f vSize = camera.getView().getSize();
    const sf::Vector2f vCenter = camera.getView().getCenter();

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const Vec3f worldPos = atomStorage.pos(i) + box.start;
        const sf::Vector2i atomScreen = camera.worldToScreen(worldPos);
        if (pointInRect(atomScreen, start, end)) {
            selectedIndices.insert(i);
            atomStorage.setSelected(i, true);
        }
    }
}

void PickingSystem::processLasso(std::span<sf::Vector2i> points, bool cumulative) {
    if (points.size() < 3) return;
    if (!cumulative) clearSelection();

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const Vec3f worldPos = atomStorage.pos(i) + box.start;
        const sf::Vector2i screenPos = camera.worldToScreen(worldPos);
        if (pointInPolygon(screenPos, points)) {
            selectedIndices.insert(i);
            atomStorage.setSelected(i, true);
        }
    }
}

void PickingSystem::handleAtomRemoval(std::size_t index) {
    selectedIndices.erase(index);
    
    std::size_t movedFrom = atomStorage.size();

    if (index < movedFrom) {
        if (selectedIndices.erase(movedFrom) > 0) {
            selectedIndices.insert(index);
            atomStorage.setSelected(index, true);
        }
    }
}

bool PickingSystem::pickAtom(sf::Vector2i screenPos, float tolerance, AtomHit& hit) const {
    switch (camera.getMode()) {
        case Camera::Mode::Mode2D:
            return pickAtom2D(screenPos, tolerance, hit);
        case Camera::Mode::Orbit:
        case Camera::Mode::Free:
            return pickAtom3D(screenPos, hit);
    }
}

bool PickingSystem::pickAtom2D(sf::Vector2i screenPos, float tolerance, AtomHit& hit) const {
    float bestDistSqr = std::numeric_limits<float>::max();
    std::size_t bestIndex = static_cast<std::size_t>(-1);

    for (std::size_t i = 0; i < atomStorage.size(); ++i) {
        const Vec3f worldPos = atomStorage.pos(i) + box.start;
        const sf::Vector2i atomScreen = camera.worldToScreen(worldPos);
        const float distSqr = (atomScreen - screenPos).lengthSquared();

        // радиус атома в экранных пикселях
        const float atomRadius = AtomData::getProps(atomStorage.type(i)).radius;
        const float screenRadius = atomRadius * camera.getZoom() + tolerance;

        if (distSqr < screenRadius * screenRadius && distSqr < bestDistSqr) {
            bestDistSqr = distSqr;
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
        const Vec3f worldPos = atomStorage.pos(i) + box.start;
        const float radius = AtomData::getProps(atomStorage.type(i)).radius;

        RaySphereHit rayHit;
        if (raySphereIntersect(ray, worldPos, radius, rayHit)) {
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
bool PickingSystem::pointInPolygon(sf::Vector2i point, std::span<sf::Vector2i> polygon) {
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
    return (minX <= point.x && point.x <= maxX && minY <= point.y && point.y <= maxY);
}
