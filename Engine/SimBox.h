#pragma once

#include "math/Vec3f.h"
#include "NeighborSearch/SpatialGrid.h"

class SimBox {
    public:
        SimBox(Vec3f size);
        bool setSizeBox(const Vec3f& newSize, int cellSize = -1);
        SpatialGrid grid;
        Vec3f size;
};
