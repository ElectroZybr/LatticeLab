#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include <glm/vec3.hpp>

#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/physics/Atom/AtomStorage.h"

class OctreeNode {
public:
    OctreeNode() : size(0.0f), charge(0.0f), center(0.0f), dipoleMoment(0.0f), parent(nullptr) {}
    explicit OctreeNode(const glm::vec3& center_) : size(0.0f), charge(0.0f), center(center_), dipoleMoment(0.0f), parent(nullptr) {}

    void build(const AtomStorage& atoms, const SpatialGrid& grid);
    void accumulateForceOnAtom(const AtomStorage& atoms, size_t atomIndex, float theta, float& forceX, float& forceY, float& forceZ,
                               float& potentialEnergy) const;
    void show() const { showNode(0); }

private:
    static constexpr int maxDepth = 2;
    static constexpr int maxParticlesPerLeaf = 8;

    float size;         // длина ребра куба
    float charge;       // суммарный заряд в узле
    glm::vec3 center;       // центр куба
    glm::vec3 dipoleMoment; // дипольный момент узла
    OctreeNode* parent;
    std::unique_ptr<OctreeNode> children[8]; // указатели на дочерние узлы

    size_t firstAtom = 0;
    size_t atomCount = 0;

    int childIndexFor(const glm::vec3& center, const glm::vec3& pos);
    void buildNode(const AtomStorage& atoms, int levels);
    void showNode(int depth) const;
};
