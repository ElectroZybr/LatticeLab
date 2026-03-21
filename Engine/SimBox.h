#pragma once

#include "math/Vec3D.h"
#include "physics/SpatialGrid.h"

class SimBox {
    public:
        SimBox(Vec3D start, Vec3D end);
        bool setSizeBox(Vec3D newStart = Vec3D(0, 0, 0), Vec3D newEnd = Vec3D(50, 50, 3), int cellSize = -1);
        SpatialGrid grid;
        Vec3D start;
        Vec3D end;
};
