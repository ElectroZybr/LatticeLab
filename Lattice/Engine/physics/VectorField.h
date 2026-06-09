#pragma once
#include <vector>

#include <glm/glm.hpp>

class AtomStorage;
class ForceField;
class SpatialGrid;

class VectorField {
public:
    VectorField(glm::ivec3 size, int sliceZ = 0);
    void resize(glm::ivec3 newSize, int newSliceZ = 0);
    void setSliceZ(int newSliceZ);
    void compute(ForceField& forceField, AtomStorage& atoms, SpatialGrid& grid);
    [[nodiscard]] glm::ivec3 gridSize() const noexcept { return size; }
    [[nodiscard]] int zSlice() const noexcept { return sliceZ; }
    [[nodiscard]] float cellScale() const noexcept { return scale; }
    [[nodiscard]] const std::vector<float>& values() const noexcept { return field; }
    float potentialAt(int x, int y) const {
        return field[x + size.x * y];
    }
    void show() const;
private:
    glm::ivec3 size;
    int sliceZ = 0;
    float scale = 1.0f;
    std::vector<float> field;
};
