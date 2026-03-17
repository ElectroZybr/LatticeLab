#include "SimBox.h"
#include "Renderer.h"

#include <algorithm>

SimBox::SimBox(Vec3D s, Vec3D e)
    : start(s),
      end(e),
      grid(std::max(1, static_cast<int>(e.x - s.x)),
           std::max(1, static_cast<int>(e.y - s.y))) {}

void SimBox::setRenderer(Renderer* r) {
    render = r;
    if (render) {
        render->wallImage(start, end);
    }
}

bool SimBox::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
    bool resized = false;

    const int newW = std::max(1, static_cast<int>(newEnd.x - newStart.x));
    const int newH = std::max(1, static_cast<int>(newEnd.y - newStart.y));
    const int oldW = std::max(1, static_cast<int>(end.x - start.x));
    const int oldH = std::max(1, static_cast<int>(end.y - start.y));
    const bool sizeChanged = (newW != oldW) || (newH != oldH);
    const bool cellSizeChanged = (cellSize > 0 && cellSize != grid.cellSize);

    if (sizeChanged || cellSizeChanged) {
        grid.resize(newW, newH, cellSize);
        resized = true;
    }

    start = newStart;
    end = newEnd;

    if (render && sizeChanged) {
        render->wallImage(start, end);
    }

    return resized;
}
