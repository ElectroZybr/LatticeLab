#include "SimBox.h"

SimBox::SimBox(Vec3f size)
    : size(size),
      grid(size.x, size.y, size.z)
    {}

bool SimBox::setSizeBox(Vec3f newSize, int cellSize) {
    bool resized = false;

    const bool sizeChanged = (newSize.x != size.x) || (newSize.y != size.y) || (newSize.z != size.z);
    const bool cellSizeChanged = (cellSize > 0 && cellSize != grid.cellSize);

    if (sizeChanged || cellSizeChanged) {
        grid.resize(newSize.x, newSize.y, newSize.z, cellSize);
        resized = true;
    }

    size = newSize;

    return resized;
}
