#pragma once

#include "math/Vec3f.h"
#include "NeighborSearch/SpatialGrid.h"
#include "Signals/Signals.h"

class SimBox: public Signals::Trackable {
    public:
        SimBox(Vec3f size);
        bool setSizeBox(const Vec3f& newSize, int cellSize = -1);
        SpatialGrid grid;
        Vec3f size;
};
