#pragma once

#include "math/Vec3f.h"
#include "physics/SpatialGrid.h"

class SimBox {
    public:
        SimBox(Vec3f start, Vec3f end);
        bool setSizeBox(Vec3f newStart = Vec3f(0, 0, 0), Vec3f newEnd = Vec3f(50, 50, 3), int cellSize = -1);
        SpatialGrid grid;
        Vec3f start;
        Vec3f end;
};
