#pragma once

#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/math/Vec3f.h"

class SimBox {
public:
    SimBox(Vec3f size);
    bool setSizeBox(const Vec3f& newSize, int cellSize = -1);
    SpatialGrid grid;
    Vec3f size;
};
