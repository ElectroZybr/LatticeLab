#include "VectorField.h"

#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/physics/Atom/AtomStorage.h"
#include "Engine/physics/ForceField.h"

#include <algorithm>
#include <iostream>

VectorField::VectorField(glm::ivec3 size, int sliceZ)
    : size(0), sliceZ(0) {
    resize(size, sliceZ);
}

void VectorField::resize(glm::ivec3 newSize, int newSliceZ) {
    size = glm::max(newSize, glm::ivec3(1));
    field.assign(static_cast<size_t>(size.x) * static_cast<size_t>(size.y), 0.0f);
    setSliceZ(newSliceZ);
}

void VectorField::setSliceZ(int newSliceZ) {
    sliceZ = std::clamp(newSliceZ, 0, std::max(0, size.z - 1));
}

void VectorField::compute(ForceField& forceField, AtomStorage& atoms, SpatialGrid& grid) {
    const float z = static_cast<float>(sliceZ) * scale;
    for (int y = 0; y < size.y; ++y) {
        for (int x = 0; x < size.x; ++x) {
            field[x + size.x * y] = forceField.coulombForceField_.PeAtPoint(
                atoms,
                grid,
                static_cast<float>(x) * scale,
                static_cast<float>(y) * scale,
                z
            );
        }
    }
}

void VectorField::show() const {
    for (int y = 0; y < size.y; ++y) {
        for (int x = 0; x < size.x; ++x) {
            std::cout << potentialAt(x, y) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "Layer " << sliceZ << " end\n";
}
