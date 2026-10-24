#pragma once

#include <algorithm>
#include <optional>

#include <glm/glm.hpp>
#include "RenderTypes.hpp"

enum class ViewportSizeMode {
    Fixed,
    Fill
};

/// Rectangle of a surface with the cursor over it: geometry only, no GPU.
/// Camera controllers read it (size, local cursor).
class ViewportArea {
public:
    glm::uvec2 position() const noexcept { return position_; }
    void setPosition(glm::uvec2 position) noexcept { position_ = position; }

    glm::uvec2 size() const noexcept { return size_; }
    void setSize(glm::uvec2 size) noexcept {
        size_ = size;
        sizeMode_ = ViewportSizeMode::Fixed;
    }

    void fitSurface() noexcept { sizeMode_ = ViewportSizeMode::Fill; }

    /// Higher layers are drawn later and receive the cursor first (GUI panels over a scene).
    int layer() const noexcept { return layer_; }
    void setLayer(int layer) noexcept { layer_ = layer; }

    /// Whether a surface position falls inside this area.
    bool contains(glm::vec2 position, glm::uvec2 surfaceSize) const noexcept {
        Rect rect;
        return resolveRect(surfaceSize, rect) &&
            position.x >= float(rect.x) && position.x < float(rect.x + rect.width) &&
            position.y >= float(rect.y) && position.y < float(rect.y + rect.height);
    }

    /// Pixel rectangle on a surface of this size; false when nothing is visible.
    bool resolveRect(glm::uvec2 surfaceSize, Rect& rect) const noexcept {
        const auto origin = position();
        if (origin.x >= surfaceSize.x || origin.y >= surfaceSize.y)
            return false;
        const auto available = surfaceSize - origin;
        const auto requested = sizeMode_ == ViewportSizeMode::Fill ? available : size();
        const glm::uvec2 extent{std::min(requested.x, available.x), std::min(requested.y, available.y)};
        if (!extent.x || !extent.y)
            return false;
        rect = {origin.x, origin.y, extent.x, extent.y};
        return true;
    }

    /// Cursor in local coordinates; valid while hovered().
    glm::vec2 cursor() const noexcept { return cursor_; }
    bool hovered() const noexcept { return hovered_; }
    /// Called by the surface owner: local cursor position, or nullopt when the cursor is elsewhere.
    void hover(std::optional<glm::vec2> local) noexcept {
        hovered_ = local.has_value();
        if (local) cursor_ = *local;
    }

private:
    glm::uvec2 position_{};
    glm::uvec2 size_{1280, 720};
    ViewportSizeMode sizeMode_ = ViewportSizeMode::Fill;
    int layer_ = 0;
    glm::vec2 cursor_{};
    bool hovered_ = false;
};
