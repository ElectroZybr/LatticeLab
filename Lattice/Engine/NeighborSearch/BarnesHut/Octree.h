#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include "Engine/math/Vec3.h"


class Octree {
public:
    Octree(const Vec3f& center, float size) : center(center), size(size) {}

    void build(const std::vector<Vec3f>& positions, const std::vector<float>& charges) {
        buildNode(0);
    }

    void buildNode(int depth) {
        if (depth < maxDepth) {
            for (int i = 0; i < 8; ++i) {
                Vec3f offset(
                    (i & 1 ? size / 4 : -size / 4),
                    (i & 2 ? size / 4 : -size / 4),
                    (i & 4 ? size / 4 : -size / 4));
                children[i] = std::make_unique<Octree>(center + offset, size / 2);
                children[i]->buildNode(depth + 1);
            }
        }
    }

    void show() const {
        showNode(0);
    }

    void showNode(int depth) const {
        for (int i = 0; i < 8; ++i) {
            if (children[i]) {
                std::string indent(depth * 2, ' ');
                std::cout << indent << i << std::endl;
                children[i]->showNode(depth + 1);
            }
        }
    }

private:
    static constexpr int maxDepth = 4;
    static constexpr int maxParticlesPerLeaf = 8;

    int depth;          // глубина узла в дереве
    float size;         // длина ребра куба
    float charge;       // суммарный заряд в узле
    Vec3f center;       // центр куба
    Vec3f dipoleMoment; // дипольный момент узла
    std::unique_ptr<Octree> children[8]; // указатели на дочерние узлы
};