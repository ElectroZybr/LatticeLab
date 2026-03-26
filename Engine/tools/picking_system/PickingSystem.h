#pragma once

#include <cstddef>
#include <unordered_set>

#include <SFML/System/Vector2.hpp>

#include "Engine/selection/OverlayState.h"
#include "Engine/physics/AtomStorage.h"
#include "Rendering/camera/Camera.h"

class SimBox;

struct AtomHit {
    std::size_t index;
    float distance;
};

class PickingSystem {
public:
    PickingSystem(Camera& camera, AtomStorage& atomStorage, SimBox& box);

    void clearSelection();

    bool pickAtom(sf::Vector2i screenPos, float tolerance, AtomHit& hit) const;

    void processClick(sf::Vector2i screenPos, bool cumulative = false);
    void processRect(sf::Vector2i start, sf::Vector2i end, bool cumulative = false);
    void processLasso(std::span<sf::Vector2i> points, bool cumulative = false);

    void handleAtomRemoval(std::size_t removedIndex);

    const std::unordered_set<std::size_t>& getSelectedIndices() const { return selectedIndices; }
    const OverlayState& getOverlay() const { return overlay; }
    OverlayState&       getOverlay()       { return overlay; }
private:
    Camera&       camera;
    AtomStorage&  atomStorage;
    SimBox& box;
    OverlayState  overlay;
    std::unordered_set<std::size_t> selectedIndices;

    // 2D пикинг одного атома — расстояние в экранных координатах
    bool pickAtom2D(sf::Vector2i screenPos, float tolerance, AtomHit& hit) const;
    // 3D пикинг одного атома — ray cast
    bool pickAtom3D(sf::Vector2i screenPos, AtomHit& hit) const;

    // Проверка точки внутри фигуры
    static bool pointInPolygon(sf::Vector2i point, std::span<sf::Vector2i> polygon);
    static bool pointInRect(sf::Vector2i point, sf::Vector2i start, sf::Vector2i end);
};