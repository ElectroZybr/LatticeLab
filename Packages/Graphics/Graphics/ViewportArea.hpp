#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <WGPU/Types.hpp>

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
    bool contains(glm::vec2 position, glm::uvec2 surfaceSize) const noexcept;

    /// Pixel rectangle on a surface of this size; false when nothing is visible.
    bool resolveRect(glm::uvec2 surfaceSize, WGPU::Rect& rect) const noexcept;

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
