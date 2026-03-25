#include <algorithm>

#include "SimBox.h"

SimBox::SimBox(Vec3f s, Vec3f e)
    : start(s),
      end(e),
      grid(std::max(1, static_cast<int>(e.x - s.x)),
           std::max(1, static_cast<int>(e.y - s.y)),
           std::max(1, static_cast<int>(e.z - s.z))
        ) {}

bool SimBox::setSizeBox(Vec3f newStart, Vec3f newEnd, int cellSize) {
    bool resized = false;

    const int newW = std::max(1, static_cast<int>(newEnd.x - newStart.x));
    const int newH = std::max(1, static_cast<int>(newEnd.y - newStart.y));
    const int newZ = std::max(1, static_cast<int>(newEnd.z - newStart.z));
    const int oldW = std::max(1, static_cast<int>(end.x - start.x));
    const int oldH = std::max(1, static_cast<int>(end.y - start.y));
    const int oldZ = std::max(1, static_cast<int>(end.z - start.z));
    const bool sizeChanged = (newW != oldW) || (newH != oldH) || (newZ != oldZ);
    const bool cellSizeChanged = (cellSize > 0 && cellSize != grid.cellSize);

    if (sizeChanged || cellSizeChanged) {
        grid.resize(newW, newH, newZ, cellSize);
        resized = true;
    }

    start = newStart;
    end = newEnd;

    return resized;
}
