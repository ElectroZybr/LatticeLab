#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include "Engine/math/Vec3.h"


class Octree {
public:
    Octree(const Vec3f& center, float size) : center(center), size(size) {}

    void build(const AtomStorage& atoms) { buildNode(atoms, 0); }
    void show() const { showNode(0); }

private:
    static constexpr int maxDepth = 2;
    static constexpr int maxParticlesPerLeaf = 8;

    int depth;          // глубина узла в дереве
    float size;         // длина ребра куба
    float charge;       // суммарный заряд в узле
    Vec3f center;       // центр куба
    Vec3f dipoleMoment; // дипольный момент узла
    std::unique_ptr<Octree> children[8]; // указатели на дочерние узлы

    void buildNode(const AtomStorage& atoms, int depth) {
        if (depth < maxDepth) {
            for (int i = 0; i < 8; ++i) {
                // проход вперед (создание дочерних узлов)
                Vec3f offset(
                    (i & 1 ? size / 4 : -size / 4),
                    (i & 2 ? size / 4 : -size / 4),
                    (i & 4 ? size / 4 : -size / 4));
                children[i] = std::make_unique<Octree>(center + offset, size / 2);
                children[i]->buildNode(atoms, depth + 1);
                // проход назад (заплнение зарядов)
                charge += children[i]->charge;
                dipoleMoment += children[i]->dipoleMoment + offset * children[i]->charge;
            }
        } else {
            // Листовой узел, считаем заряд и дипольный момент
            for (size_t i = 0; i < atoms.size(); ++i) {
                Vec3f pos(atoms.posX(i), atoms.posY(i), atoms.posZ(i));
                float q = atoms.charge(i);
                charge += q;
                dipoleMoment += pos * q;
            }
        }
    }

    void showNode(int depth) const {
        for (int i = 0; i < 8; ++i) {
            if (children[i]) {
                std::string indent(depth * 2, ' ');
                std::cout << indent << i << ", " << charge <<std::endl;
                children[i]->showNode(depth + 1);
            }
        }
    }
};