#include "Viewport.hpp"

#include <algorithm>

bool Viewport::contains(glm::vec2 position, glm::uvec2 surfaceSize) const noexcept {
    WGPU::Rect rect;
    return resolveRect(surfaceSize, rect) &&
        position.x >= float(rect.x) && position.x < float(rect.x + rect.width) &&
        position.y >= float(rect.y) && position.y < float(rect.y + rect.height);
}

bool Viewport::resolveRect(glm::uvec2 surfaceSize, WGPU::Rect& rect) const noexcept {
    const auto origin = position();
    if (origin.x >= surfaceSize.x || origin.y >= surfaceSize.y)
        return false;

    const auto available = surfaceSize - origin;
    const auto requested = sizeMode_ == ViewportSizeMode::Fill ? available : size();
    const glm::uvec2 extent{
        std::min(requested.x, available.x),
        std::min(requested.y, available.y)
    };

    if (!extent.x || !extent.y)
        return false;

    rect = {origin.x, origin.y, extent.x, extent.y};
    return true;
}
