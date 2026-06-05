#include "Octree.h"

#include <cmath>

#include "Engine/Consts.h"
#include "Engine/physics/ForceFields/CoulombForceField.h"

void OctreeNode::build(const AtomStorage& atoms, const SpatialGrid& grid){
    const auto maxSide = std::max(grid.size.x, std::max(grid.size.y, grid.size.z));
    int levels = static_cast<int>(std::ceil(std::log2(static_cast<float>(maxSide))));
    size = std::pow(2.0f, static_cast<float>(levels)) * grid.cellSize;
    center = glm::vec3(size * 0.5f);
    firstAtom = 0;
    atomCount = atoms.size();
    buildNode(atoms, levels); 
}

void OctreeNode::buildNode(const AtomStorage& atoms, int levels) {
    charge = 0.0f;
    dipoleMoment = glm::vec3(0.0f);
    if (levels > 0) {
        size_t octantCounts[8] = {};
        const size_t begin = firstAtom;
        const size_t end = firstAtom + atomCount;

        // Morton-порядок уже сгруппировал атомы так, что на данном уровне октанты идут подряд
        for (size_t atomIndex = begin; atomIndex < end; ++atomIndex) {
            const glm::vec3 pos(atoms.posX(atomIndex), atoms.posY(atomIndex), atoms.posZ(atomIndex));
            ++octantCounts[childIndexFor(center, pos)];
        }

        size_t octantFirst[8];
        size_t runningFirst = firstAtom;
        for (int i = 0; i < 8; ++i) {
            octantFirst[i] = runningFirst;
            runningFirst += octantCounts[i];
        }

        for (int i = 0; i < 8; ++i) {
            if (octantCounts[i] == 0) {
                continue;
            }

            // проход вперед (создание дочерних узлов)
            glm::vec3 offset(
                (i & 1 ? size / 4 : -size / 4),
                (i & 2 ? size / 4 : -size / 4),
                (i & 4 ? size / 4 : -size / 4));
            children[i] = std::make_unique<OctreeNode>(center + offset);
            children[i]->parent = this;
            children[i]->size = size / 2;
            children[i]->firstAtom = octantFirst[i];
            children[i]->atomCount = octantCounts[i];
            children[i]->buildNode(atoms, levels - 1);
            // проход назад (заплнение зарядов)
            charge += children[i]->charge;
            dipoleMoment += children[i]->dipoleMoment;
        }
    } else {
        // Листовой узел, считаем заряд и дипольный момент
        const size_t begin = firstAtom;
        const size_t end = firstAtom + atomCount;
        for (size_t i = begin; i < end; ++i) {
            glm::vec3 pos(atoms.posX(i), atoms.posY(i), atoms.posZ(i));
            float q = atoms.charge(i);
            charge += q;
            dipoleMoment += pos * q;
        }
    }
}

int OctreeNode::childIndexFor(const glm::vec3& center, const glm::vec3& pos) {
    int index = 0;
    if (pos.x >= center.x) {
        index |= 1;
    }
    if (pos.y >= center.y) {
        index |= 2;
    }
    if (pos.z >= center.z) {
        index |= 4;
    }
    return index;
}

void OctreeNode::showNode(int depth) const {
    for (int i = 0; i < 8; ++i) {
        if (children[i]) {
            std::string indent(depth * 2, ' ');
            std::cout << indent << i << ", " << charge <<std::endl;
            children[i]->showNode(depth + 1);
        }
    }
}

void OctreeNode::accumulateForceOnAtom(const AtomStorage& atoms, size_t atomIndex, float theta, float& forceX, float& forceY, float& forceZ,
                                       float& potentialEnergy) const {
    if (atomCount == 0 || charge == 0.0f) {
        return;
    }

    const float chargeA = atoms.charge(atomIndex);
    if (chargeA == 0.0f) {
        return;
    }

    const bool isLeaf = std::all_of(std::begin(children), std::end(children), [](const auto& child) { return child == nullptr; });
    const bool containsTarget = atomIndex >= firstAtom && atomIndex < firstAtom + atomCount;

    if (isLeaf) {
        const float posX = atoms.posX(atomIndex);
        const float posY = atoms.posY(atomIndex);
        const float posZ = atoms.posZ(atomIndex);

        for (size_t other = firstAtom; other < firstAtom + atomCount; ++other) {
            if (other == atomIndex) {
                continue;
            }

            const float dx = atoms.posX(other) - posX;
            const float dy = atoms.posY(other) - posY;
            const float dz = atoms.posZ(other) - posZ;
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 <= Consts::Epsilon) {
                continue;
            }

            const float qqScale = CoulombForceField::kCoulombEvAngstrom * chargeA * atoms.charge(other);
            const float invR = 1.0f / std::sqrt(d2);
            const float forceScale = qqScale * invR / d2;

            forceX -= dx * forceScale;
            forceY -= dy * forceScale;
            forceZ -= dz * forceScale;
            potentialEnergy += 0.5f * qqScale * invR;
        }
        return;
    }

    const glm::vec3 sourcePos = dipoleMoment / charge;
    const float dx = sourcePos.x - atoms.posX(atomIndex);
    const float dy = sourcePos.y - atoms.posY(atomIndex);
    const float dz = sourcePos.z - atoms.posZ(atomIndex);
    const float d2 = dx * dx + dy * dy + dz * dz;

    if (!containsTarget && d2 > Consts::Epsilon) {
        const float theta2 = theta * theta;
        if ((size * size) <= theta2 * d2) {
            const float qqScale = CoulombForceField::kCoulombEvAngstrom * chargeA * charge;
            const float invR = 1.0f / std::sqrt(d2);
            const float forceScale = qqScale * invR / d2;

            forceX -= dx * forceScale;
            forceY -= dy * forceScale;
            forceZ -= dz * forceScale;
            potentialEnergy += 0.5f * qqScale * invR;
            return;
        }
    }

    for (const auto& child : children) {
        if (child) {
            child->accumulateForceOnAtom(atoms, atomIndex, theta, forceX, forceY, forceZ, potentialEnergy);
        }
    }
}
