#include <algorithm>

#include "SimBox.h"
#include "Rendering/BaseRenderer.h"

SimBox::SimBox(Vec3D s, Vec3D e)
    : start(s),
      end(e),
      grid(std::max(1, static_cast<int>(e.x - s.x)),
           std::max(1, static_cast<int>(e.y - s.y)),
           std::max(1, static_cast<int>(e.z - s.z))
        ) {}

void SimBox::setRenderer(IRenderer* r) {
    render = r;
    if (render) {
        render->wallImage(start, end);
    }
}

bool SimBox::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
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

    if (render && sizeChanged) {
        render->wallImage(start, end);
    }

    return resized;
}
